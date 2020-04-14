#pragma once

#ifndef Microsha_hpp
#define Microsha_hpp

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

using namespace std;

// коды возврата команд
enum RET_CODES
{
    R_SUCCESS,
    R_FAILURE,
    R_EXIT
};

// на перспективу...
// команда с привязанными дескрипторами ввода, вывода и ошибок
struct cmd
{
    int fd_in;
    int fd_out;
    int fd_err;
    vector<string> command;
};

// Описание класса оболочки
class Microsha
{
public:
    Microsha(const char**);                        // конструктор принимающий указатель на переменные окружения
    
    RET_CODES ExecCmd(string cmd);                // метод исполнения элементарной команды
    void Invite();

private:
    uid_t uid;                                    // уникальный идентификатор пользователя
    string home_dir;                            // домашняя дирректория пользователя

    map<string, string> vars;                    // переменные окружения

    vector<string> split(string);                // метод разделения входной строки по пробелам
    vector<struct cmd> parse(vector<string>);    // метод разбора строки (разбитой по пробелам)
    RET_CODES make(vector<string>);                // метод исполнения элементарной команды
    
    // внутренние команды
    void cd(string);                            // войти в дирректорию
    void pwd();                                    // печать рабочей дирректории
    void time(vector<string>);                    // замер времени выполнения команды
    void echo(string);                            // вывод строки
    void set(string);                            // задать переменную окружения

    string get_all_files(size_t);                // метод получения списка файлов в дирректории
};

#endif /* Microsha_hpp */
