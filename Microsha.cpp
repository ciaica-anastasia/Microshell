#include "Microsha.hpp"

// конструктор
Microsha::Microsha(const char** envp)
{
    struct passwd *passwd_entry = nullptr;
    
    uid = getuid();                        // получаем uid пользователя
    passwd_entry = getpwuid(uid);        // получаем запись passwd пользователя
    home_dir = passwd_entry->pw_dir;    // извлекаем домашнюю дирректорию

    // копируем переменные окружения
    while (*envp)
    {
        size_t len = strlen(*envp);
        int i = 0;

        while (i < len && (*envp)[i] != '=') ++i;        // определяем позицию знака '='
        
        string key(*envp, i);                            // копируем все до "=" как ключ
        string val(*envp + i + 1);                        // после - как значение

        vars.insert(pair<string, string>(key, val));    // вставляем пару в словарь
        ++envp;
    }
}

void Microsha::Invite()
{
    char cwd[1024] = { 0 };
    if (getcwd(cwd, sizeof(cwd)) == NULL)                // получаем текущую рабочую директорию
        throw "Can't get current working directory";

    cout << cwd << ((uid) ? "> " : "! ");                // если uid = 0 -> пользователь root
}

RET_CODES Microsha::make(vector<string> command)        // выполнение элементарной команды
{
    size_t command_size = command.size();
    
    if (!command_size)                                // если пришла пустая команда
        return RET_CODES::R_FAILURE;
    
    if (command[0] == "cd")
        cd(command_size > 1 ? command[1] : "");
    else if (command[0] == "pwd")
        pwd();
    else if (command[0] == "time")
        time(command);
    else if (command[0] == "echo")
        echo(command_size > 1 ? command[1] : "");
    else if (command[0] == "set")
        set(command_size > 1 ? command[1] : "");
    else if (command[0] == "exit" || command[0] == "quit")
        return R_EXIT;
    else                                                         // external command
    {
        pid_t pid = fork();                                        // раздваиваем процесс
        
        if (pid > 0) // parent
        {
            int status = 0;
            wait(&status);                                        // ждем всех потомков
            
            if(!WIFEXITED(status))
                perror("kind proc");
        }
        else if (pid == 0) // child
        {
            // формируем аргументы внешней команды
            
            char** args = (char**)malloc((command_size + 1) * sizeof(char*));
            args[command_size] = NULL;
            
            for (int i = 0; i < command_size; ++i)
                args[i] = (char*)command[i].c_str();
            
            // запуск внешней программы
            // новый процесс замещает текущий
            execvp(command[0].c_str(), args);
            
            free(args);
            perror("execv");
            exit(-1);                // в случае ошибки завершаем работу
        }
        else
            perror("fork");
    }
    
    return RET_CODES::R_SUCCESS;
}

string Microsha::get_all_files(size_t len)    // чтение текущей директории
{
    string files = "";
    
    DIR *d;
    struct dirent *dir;
    
    d = opendir(".");
    if (d)                // если получилось открыть директорию
    {
        while ((dir = readdir(d)) != NULL) // пока не просмотрели до конца
        {
            if (!len || (len && strlen(dir->d_name) == len))
            {
                files.append(dir->d_name); // добавляем в строку текущий файл
                files.append(" ");
            }
        }
        closedir(d); // не забываем закрыть
    }
    
    return files;
}

vector<string> Microsha::split(string cmd)
{
    stringstream temp(cmd);        // открываем строковый поток с входной командой
    vector<string> lexems;
    string str;
    
    // пока есть данные в потоке считываем до следующего пробельного символа
    while (temp >> str)
    {
        if (str == "*")
        {
            stringstream files(get_all_files(0)); //получаем содержимое текущей директории и помещаем в поток
            while (files >> str) // пока поток не пуст добавляем имена в вектор лексем
            {
                lexems.push_back(str);
            }
    continue;
        }
        else if (str[0] == '$') //если встретилась переменная окружения
        {//ищем ее значение в словаре
            map<string,string>::iterator it = vars.find(str.substr(1));
            
         //если значение не найдено оставляем пустым
            if (it != vars.end())
                lexems.push_back(it->second);
            else
                lexems.push_back(" ");
            continue;
        }
        lexems.push_back(str);
    }
        
    return lexems;
}

// command1 | command2 param1 param2 < file | command3 > file_out

vector<struct cmd> Microsha::parse(vector<string> lexems)    // разбор строки
{
    vector<struct cmd> commands;
    
    string str;
    int state = 0;
    
    for (auto s : lexems)    // проверяем каждую лексемму
    {
        vector<string> command;
        
        if (s[0] == '"')
        {
            if (state)
            {
                // если встретили кавычку, удаляем ее и меняем состояние
                command.push_back(str.erase(str.size() - 1, 1));
                str = "";
            }
            state = !state;
        }
        else if (state)
        {
            // накапливаем значения в случае встреченной первой кавычки
            str.append(s);
            str.append(" ");
        }
        else if (s == ">"){}
        else if (s == "<"){}
        else if (s == "*" || s[0] == '?')
        {
            size_t count = 0;
            //if (s[0] == '?')
            
            // меняем '*' на список содержимого директории через пробел
            command.push_back(get_all_files(count));
        }
        else if (s == "|")
        {
            //commands.push_back(command);
            continue;
        }
        else if (s[0] == '$') // если встретилась переменная окружения
        {
            // ищем ее значение в словаре
            map<string, string>::iterator it = vars.find(s.substr(1));
            
            // если значение не найдено оставляем пустым
            if (it != vars.end())
                command.push_back(it->second);
            else
                command.push_back(" ");
        }
    }
    
    return commands;
}

RET_CODES Microsha::ExecCmd(string cmd)
{
    vector<string> temp = split(cmd);     // разбиваем строку по пробелам
    RET_CODES ret = R_FAILURE;
    
    //for(auto command : parse(temp)) // выполнение каждой команды после разбора
    //{
        // dup2 (in,out,err)
        ret = make(temp); // выполнить простейшую команду
        // dup2 back (in,out,err)
        
        // завершаем выполнение, если какая-либо команда не была удачно завершена
        //if (ret != R_SUCCESS)
        //    return ret;
    //}
    
    return ret;
}

// Внутринние команды оболочки

// изменить рабочую директорию
void Microsha::cd(string path)
{
    if (path.length() == 0) path = home_dir; // если путь не задан, то возвращаемся домой
    if (chdir(path.c_str())) perror("chdir"); // если не удалось изменить директорию, выводим ошибку
}

// печать текущей рабочей директории
void Microsha::pwd() // print current working directory (cwd)
{
    char cwd[1024] = { 0 };
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        throw "Can't get current working directory";
    cout << cwd << endl;
}

// вывод на экран сообщения
void Microsha::echo(string str) { cout << str; }

// вычисление времени работы программы
void Microsha::time(vector<string> command)
{
    /*clock_t start;
    
    pid_t pid = fork();
    
    if (pid > 0) // parent
    {
        int status = 0;
        wait(&status);
        
        if(!WIFEXITED(status))
            perror("kind proc");
    }
    else if (pid == 0) // child
    {
        char** args = (char**)malloc((command_size + 1) * sizeof(char*));
        args[command_size] = NULL;
        
        for (int i = 0; i < command_size; ++i)
            args[i] = (char*)command[i].c_str();
        
        execvp(command[0].c_str(), args);
        
        free(args);
        perror("execv");
        exit(-1);
    }
    else
        perror("fork");
         */
}

// Создание или изменение переменной окружения
void Microsha::set(string str)
{
    size_t pos = str.find("=");
    
    if (pos == string::npos) // если найден знак '='
    {
        // если такого ключа (переменной) нет, вставляем с пустым значением
        if (vars.find(str) == vars.end())
            vars.insert(pair<string, string>(str, ""));
        // vars[str] = "";
        return;
    }
    
    // иначе разбиваем строку до знака и после на пару (key, value)
    string key(str.begin(), str.begin() + pos);
    string value(str, pos + 1);
    
    if (vars.find(key) != vars.end())    // если ключ найден, меняем значение
        vars[key] = value;
    else                                // иначе вставляем новое
        vars.insert(pair<string, string>(key, value));
}
