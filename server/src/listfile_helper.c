#include "server.h"

// From https://blog.csdn.net/nyist327/article/details/38976581
const char *statbuf_get_perms(struct stat *mstat)
{
    static char perms[] = "----------";
    mode_t mode = mstat->st_mode;

    switch (mode & S_IFMT)
    {
    case S_IFSOCK:
        perms[0] = 's';
        break;
    case S_IFLNK:
        perms[0] = 'l';
        break;
    case S_IFREG:
        perms[0] = '-';
        break;
    case S_IFBLK:
        perms[0] = 'b';
        break;
    case S_IFDIR:
        perms[0] = 'd';
        break;
    case S_IFCHR:
        perms[0] = 'c';
        break;
    case S_IFIFO:
        perms[0] = 'p';
        break;
    }

    if (mode & S_IRUSR)
        perms[1] = 'r';
    if (mode & S_IWUSR)
        perms[2] = 'w';
    if (mode & S_IXUSR)
        perms[3] = 'x';
    if (mode & S_IRGRP)
        perms[4] = 'r';
    if (mode & S_IWGRP)
        perms[5] = 'w';
    if (mode & S_IXGRP)
        perms[6] = 'x';
    if (mode & S_IROTH)
        perms[7] = 'r';
    if (mode & S_IWOTH)
        perms[8] = 'w';
    if (mode & S_IXOTH)
        perms[9] = 'x';

    if (mode & S_ISUID)
        perms[3] = (perms[3] == 'x') ? 's' : 'S';
    if (mode & S_ISGID)
        perms[6] = (perms[6] == 'x') ? 's' : 'S';
    if (mode & S_ISVTX)
        perms[9] = (perms[9] == 'x') ? 't' : 'T';

    return perms;
}

const char *statbuf_get_date(struct stat *mstat)
{
    static char datebuf[1024] = {0};
    struct tm *ptm;
    time_t ct = mstat->st_ctime;
    ptm = localtime(&ct);

    const char *format = "%b %e %H:%M";

    strftime(datebuf, sizeof datebuf, format, ptm);

    return datebuf;
}

const char *statbuf_get_filename(struct stat *mstat, const char *name)
{
    static char filename[1024] = {0};
    if (S_ISLNK(mstat->st_mode))
    {
        char linkfile[1024] = {0};
        if (readlink(name, linkfile, sizeof linkfile) < 0)
        {
            return filename;
        }
        snprintf(filename, sizeof filename, " %s -> %s", name, linkfile);
    }
    else
    {
        strcpy(filename, name);
    }

    return filename;
}

const char *statbuf_get_user_info(struct stat *mstat)
{
    static char info[1024] = {0};
    snprintf(info, sizeof info, " %3lu %8d %8d", mstat->st_nlink, mstat->st_uid, mstat->st_gid);

    return info;
}

const char *statbuf_get_size(struct stat *mstat)
{
    static char buf[100] = {0};
    snprintf(buf, sizeof buf, "%8lu", (unsigned long)mstat->st_size);
    return buf;
}

