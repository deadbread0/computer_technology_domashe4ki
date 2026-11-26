#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

//родитель больше не ждёт окончания stdout, прежде чем начать читать stderr, он обслуживает оба канала в одном цикле
int main(const int argc, const char* argv[]) 
{
    int fds[2] = {};
    int fdserr[2] = {};
    char buf[20240] = {};
    char buferr[20240] = {};

    if (pipe(fds) < 0)
        perror("pipe");
    
    if (pipe(fdserr) < 0)
        perror("pipe");

    int p = fork();
    
    if (p == 0)
    {
        close(fds[0]);
        close(fdserr[0]);

        dup2(fds[1], 1);
        dup2(fdserr[1], 2);

        close(fds[1]);
        close(fdserr[1]);

        execvp(argv[1], argv + 1);

        return 0;
    }

    close(fds[1]);  //закрываем на запись (иначе eof от read не случится и мы не сможем понять,
                    //  когда все прочитали)
    close(fdserr[1]);

    // 
    long bytes = 0;
    int counter = 0;
    long new_lines = 0, words = 0;
    int in_word = 0;

    int flags = fcntl(fds[0], F_GETFL, 0);
    if (flags == -1) {
        perror("F_GETFL");
        return -1;
    }

    fcntl(fds[0], F_SETFL, flags|O_NONBLOCK);

    long byteserr = 0;
    int countererr = 0;
    long new_lineserr = 0, wordserr = 0;
    int in_word_err = 0;

    int flagserr = fcntl(fdserr[0], F_GETFL, 0);
    if (flagserr == -1) {
        perror("F_GETFL");
        return -1;
    }

    fcntl(fdserr[0], F_SETFL, flagserr|O_NONBLOCK);

    fd_set readfds;

    int stdout_open = 1;
    int stderr_open = 1;

    while (stdout_open || stderr_open)
    {
        int maxfd = -1;
        FD_ZERO(&readfds);

        if (stdout_open)
        {
            FD_SET(fds[0], &readfds);
            maxfd = fds[0];
        }

        if (stderr_open)
        {
            FD_SET(fdserr[0], &readfds);

            if (fdserr[0] > maxfd)
                maxfd = fdserr[0];
        }

        int res = select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (res < 0)
        {
            if (errno == EINTR)
                continue;

            perror("select");
            break;
        }

        if (stdout_open && FD_ISSET(fds[0], &readfds))
        {
            while (1)
            {
                counter = read(fds[0], buf, sizeof(buf));

                if (counter > 0)
                {
                    bytes += counter;

                    for (int i = 0; i < counter; i++)
                    {
                        unsigned char c = buf[i];

                        if (c == '\n')
                            new_lines++;

                        if (isspace(c))
                            in_word = 0;
                        else if (!in_word)
                        {
                            in_word = 1;
                            words++;
                        }
                    }
                }
                else if (counter == 0)
                {
                    close(fds[0]);
                    stdout_open = 0;
                    break;
                }
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                else if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    perror("read stdout");
                    close(fds[0]);
                    stdout_open = 0;
                    break;
                }
            }
        }

        if (stderr_open && FD_ISSET(fdserr[0], &readfds))
        {
            while (1)
            {
                countererr = read(fdserr[0], buferr, sizeof(buferr));

                if (countererr > 0)
                {
                    byteserr += countererr;

                    for (int i = 0; i < countererr; i++)
                    {
                        unsigned char c = buferr[i];

                        if (c == '\n')
                            new_lineserr++;

                        if (isspace(c))
                            in_word_err = 0;
                        else if (!in_word_err)
                        {
                            in_word_err = 1;
                            wordserr++;
                        }
                    }
                }
                else if (countererr == 0)
                {
                    close(fdserr[0]);
                    stderr_open = 0;
                    break;
                }
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                else if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    perror("read stderr");
                    close(fdserr[0]);
                    stderr_open = 0;
                    break;
                }
            }
        }
    }

    printf("amount of bytes: %ld\n", bytes);
    printf("amount of new lines: %ld\n", new_lines);
    printf("amount of words: %ld\n", words);


    printf("amount of byteserr: %ld\n", byteserr);
    printf("amount of new lineserr: %ld\n", new_lineserr);
    printf("amount of wordserr: %ld\n", wordserr);
    
    wait(0); 

    return 0;

}
