#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void Func(const char* path, bool* flags_arr);
void SetFlags(const int argc, const char* argv[], bool* flags_arr, int* amount_of_dir);
void PrintMode(mode_t mode);
void PrintFileInfo(struct stat st, bool* flags_arr);

enum FLAGS_T {
    ONE,
    L,
    A,
    I,
    N,
    R
};

struct FLAGS_I {
    const char* flag_name;
    enum FLAGS_T flag_num;
};

struct FLAGS_I ALL_FLAGS[] = {
    {"-1", ONE},
    {"-l", L},
    {"-a", A},
    {"-i", I},
    {"-n", N},
    {"-R", R}
};

int main(const int argc, const char* argv[])
{
    bool flags_arr[6] = {0};
    int amount_of_dir = 0;
    SetFlags(argc, argv, flags_arr, &amount_of_dir);

    if (argc == 1 || amount_of_dir == 0)
        Func(".", flags_arr);

    for (int i = 0; i < argc - 1; i++)
    {
        if (argv[i + 1][0] == '-')
            continue;
        if (amount_of_dir > 1)
            printf("%s:\n", argv[i + 1]);
        Func(argv[i + 1], flags_arr);
        if ((amount_of_dir > 1) && (amount_of_dir - i > 1))
            printf("\n\n");
    }
    printf("\n");
}

void SetFlags(const int argc, const char* argv[], bool* flags_arr, int* amount_of_dir)
{
    int amount_of_flags = sizeof(ALL_FLAGS) / sizeof(ALL_FLAGS[0]);
    for (int j = 1; j < argc; j++)
    {
        bool is_flag = false;
        for (int i = 0; i < amount_of_flags; i++)
        {
            if (strncmp(ALL_FLAGS[i].flag_name, argv[j], strlen(ALL_FLAGS[i].flag_name)) == 0)
            {
                flags_arr[ALL_FLAGS[i].flag_num] = true;
                is_flag = true;
            }
        }
        if (!is_flag && argv[j][0] != '-')
            (*amount_of_dir)++;
    }
}

void Func(const char* path, bool* flags_arr)
{
    struct stat st;
    char* mypath = calloc(strlen(path) + 2, sizeof(char));
    mypath = memcpy(mypath, path, strlen(path) + 2);
    if (lstat(mypath, &st) < 0) 
    {
        for (int i = 0; i < sizeof(path); i++)
        {
            mypath[sizeof(mypath) - i] = mypath[sizeof(mypath) - i - 2];
        }
        mypath[0] = '.';
        mypath[1] = '/';
        if (lstat(mypath, &st) < 0)
        {
            perror("lstat");
            free(mypath);
            return;
        }
    }

    const char* target_path = path;
    char* alloc_buf = NULL;
    bool is_symlink = S_ISLNK(st.st_mode);

    if (is_symlink) 
    {
        alloc_buf = (char*)calloc(1024, sizeof(char));
        ssize_t len = readlink(mypath, alloc_buf, 1023);
        if (len < 0) 
        {
            perror("readlink");
            free(alloc_buf);
            free(mypath);
            return;
        }
        alloc_buf[len] = '\0';
        
        target_path = alloc_buf;

        struct stat target_st;
        if (stat(target_path, &target_st) < 0)
        {
            perror("stat");
            free(alloc_buf);
            free(mypath);
            return;
        }
        
        if (!flags_arr[L] && !flags_arr[N] && S_ISDIR(target_st.st_mode)) 
        {
            st = target_st; 
        }
    }

    if (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)) 
    {
        if (flags_arr[N] || flags_arr[L])
            PrintFileInfo(st, flags_arr);

        printf("%s", mypath);

        if (is_symlink && (flags_arr[L] || flags_arr[N])) 
        {
            printf(" -> %s", target_path);
        }

        if (flags_arr[ONE] || flags_arr[L] || flags_arr[N])
            printf("\n");
        else
            printf("  ");
    } 
    else if (S_ISDIR(st.st_mode)) 
    {
        struct dirent **namelist;
        
        int count = scandir(target_path, &namelist, NULL, alphasort);
        if (count < 0) 
        {
            perror("scandir");
            if (alloc_buf) free(alloc_buf);
            free(mypath);
            return;
        }

        struct stat *stats = malloc(count * sizeof(struct stat));
        bool *visible = calloc(count, sizeof(bool));
        long long total_blocks = 0;

        char** subdirs = NULL;
        int subdir_count = 0;

        for (int i = 0; i < count; i++) 
        {
            if ((namelist[i]->d_name[0] != '.') || flags_arr[A]) 
            {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", target_path, namelist[i]->d_name);
                
                if (lstat(full_path, &stats[i]) == 0) 
                {
                    total_blocks += stats[i].st_blocks;
                    visible[i] = true;

                    if (flags_arr[R] && S_ISDIR(stats[i].st_mode))
                    {
                        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
                        {
                            subdirs = realloc(subdirs, sizeof(char*) * (subdir_count + 1));
                            subdirs[subdir_count] = strdup(full_path);
                            subdir_count++;
                        }
                    }
                }
            }
        }

        if (flags_arr[N] || flags_arr[L]) 
        {
            printf("total %lld\n", total_blocks / 2);
        }

        for (int i = 0; i < count; i++) 
        {
            if (!visible[i]) 
            {
                free(namelist[i]);
                continue;
            }

            if (flags_arr[N] || flags_arr[L]) 
            {
                PrintFileInfo(stats[i], flags_arr);
            }

            printf("%s", namelist[i]->d_name);

            if (flags_arr[ONE] || flags_arr[L] || flags_arr[N])
                printf("\n");
            else
                printf("  ");

            free(namelist[i]); 
        }

        if (!flags_arr[ONE] && !flags_arr[L] && !flags_arr[N]) 
        {
            printf("\n");
        }

        free(namelist);
        free(stats);
        free(visible);

        if (flags_arr[R] && subdir_count > 0)
        {
            for (int i = 0; i < subdir_count; i++)
            {
                printf("\n%s:\n", subdirs[i]);
                Func(subdirs[i], flags_arr);
                free(subdirs[i]);
            }
            free(subdirs);
        }
    }

    if (alloc_buf) 
    {
        free(alloc_buf);
    }
    free(mypath);
}

void PrintMode(mode_t mode) 
{
    if (S_ISDIR(mode))  
        printf("d");
    else if (S_ISLNK(mode)) 
        printf("l");
    else                    
        printf("-");

    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");

    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");

    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x " : "- ");
}

void PrintFileInfo(struct stat st, bool* flags_arr)
{
    PrintMode(st.st_mode);
    printf("%ld ", (long)st.st_nlink);

    if (flags_arr[L])
    {
        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        printf("%s %s ", pw->pw_name, gr->gr_name);
    }
    else
        printf("%d %d ", st.st_uid, st.st_gid);

    printf("%ld ", st.st_size);

    char *time_str = ctime(&st.st_mtime);
    if (time_str) 
    {
        time_str[strlen(time_str) - 1] = '\0'; 
        printf("%s ", time_str);
    }
}
