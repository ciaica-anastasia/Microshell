#include <iostream>
#include <string>

#include <unistd.h>
#include <dirent.h>

using namespace std;

#include "Microsha.hpp"

int main(int argc, char** argv, char** envp)
{
    Microsha m((const char**)envp); // Создание объекта оболочки
    string cmd;
    RET_CODES res = R_EXIT;

    do
    {
        m.Invite(); // Вывод приглашения для ввода

        getline(cin, cmd); // считываем команду в строку

        res = m.ExecCmd(cmd);
    } while (res != RET_CODES::R_EXIT); // Обрабатываем команды пока код возврата не будет EXIT

    return 0;
}
