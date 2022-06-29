#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

typedef struct User
{
    char *username;
    char *password;
} User;

void set_user(User *u, char *username, char *password)
{

    u->username = username;
    u->password = password;
}

int callback(void *u, int argc, char **argv, char **azColName)
{
    char *username = (char *)malloc(255);
    char *password = (char *)malloc(255);

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(azColName[i], "USERNAME") == 0)
        {
            strcpy(username, argv[i]);
        }
        else if (strcmp(azColName[i], "PASSWORD") == 0)
        {
            strcpy(password, argv[i]);
        }
    }
    set_user(u, username, password);
    return 0;
}

User *selectUser(char *name)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    char sql[255];
    const char *data = "Callback function called";

    /* Open database */
    rc = sqlite3_open("irc.db", &db);

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    snprintf(sql, sizeof(sql), "SELECT * from USERS where USERNAME='%s' LIMIT 1;", name);
    struct User *u = (struct User *)malloc(sizeof(struct User));
    u->username = NULL;
    u->password = NULL;
    rc = sqlite3_exec(db, sql, callback, u, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return NULL;
    }

    sqlite3_close(db);
    return u;
}

int insertUser(char *username, char *password)
{
    sqlite3 *db;

    char *zErrMsg = 0;
    int rc;
    char sql[255];

    /* Open database */
    rc = sqlite3_open("irc.db", &db);

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    /* Create SQL statement */
    snprintf(sql, sizeof(sql), "INSERT INTO USERS (USERNAME, PASSWORD) VALUES('%s', '%s'); ", username, password);

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 1;
    }
    sqlite3_close(db);
    return 0;
}