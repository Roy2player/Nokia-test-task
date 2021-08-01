#include <iostream>
#include <iomanip>
#include <Windows.h>

using namespace std;
#define PATH_MAX 256

struct mem_block_t {
    mem_block_t* next_mb;   // next memory block or null
    size_t mb_size;         // memory block size
    uint8_t buf[1];         // memory buffer of variable size
};

struct mem_file_img_t {
    mem_file_img_t* next_mfi; // next memory image file or null
    char path[PATH_MAX];      // file path
};

struct return_mem {
    mem_file_img_t* file_not_loaded;
    mem_file_img_t* file_loaded;
    mem_block_t* block_clear;
    mem_block_t* block_full;
};

// Копирование блока файла
mem_file_img_t* copy_mem_file_img_t(mem_file_img_t* file) {
    mem_file_img_t* new_file = new mem_file_img_t;
    strcpy_s(new_file->path, file->path);
    new_file->next_mfi = NULL;

    return new_file;
}

// Копирование блока памяти
mem_block_t* copy_mem_block_t(mem_block_t* block) {
    mem_block_t* new_block = (mem_block_t*)(new uint8_t[block->mb_size + sizeof(mem_block_t*) + sizeof(size_t)]); // Выделяем необходимое количество памяти
    new_block->mb_size = block->mb_size;
    
    for (size_t index = 0; index < block->mb_size; index++) {
        new_block->buf[index] = block->buf[index];
    }

    new_block->next_mb = NULL;

    return new_block;
}

// Добавление блока памяти в хвост списка
mem_block_t* add_mem_block_t(mem_block_t* list, const size_t size) {
    mem_block_t* new_block = (mem_block_t*)(new uint8_t[size + sizeof(mem_block_t*) + sizeof(size_t)]); // Выделяем память под новый блок
    memset(new_block->buf, 0, size); // Очищаем выделенную память под буффер
    new_block->mb_size = size; // Размер блока
    new_block->next_mb = NULL; // Следующего блока нет

    if (list) { // Если список не пустой
        mem_block_t* temp = list; // Временный указатель для перемещения по списку

        while (temp->next_mb) { // Пока не встречаем конец
            temp = temp->next_mb; // Перемещаемся по списку
        }
        temp->next_mb = new_block; // Добавить новый блок в конец списка
        
        return list; // Возвращаем новый список
    }

    list = new_block; // Инициализируем список
    
    return list; // Возвращаем инициализированный список
}

// Добавление файла в хвост спика
mem_file_img_t* add_file_img_t(mem_file_img_t* list, const char* patch) {
    mem_file_img_t* new_file_img = new mem_file_img_t; // Выделяем память под новый файл
    strcpy_s(new_file_img->path, patch); // Путь к файлу
    new_file_img->next_mfi = NULL; // Следующего файла нет

    if (list) { // Если список не пустой
        mem_file_img_t* temp = list; // Временный указатель для перемещения по списку

        while (temp->next_mfi) { // Пока не встречаем конец
            temp = temp->next_mfi; // Перемещаемся по списку
        }
        temp->next_mfi = new_file_img; // Добавляем новый файл в конец

        return list; // Возвращаем список
    }

    list = new_file_img; // Инициализируем список

    return list; // Возвращаем инициализированный список
}

// Выводим блоки памяти в консоль
void print_mem_block_t(mem_block_t* list) {
    if (!list) {
        cout << "Нет" << endl;
    }

    unsigned int index = 1; // Номер блока 

    while (list) { // Пока не встречаем конец
        cout << "Блок № " << index++ << " размер " << list->mb_size << " байт;" << endl;
        for (unsigned int count = 0; count < list->mb_size; count++) {
            cout << hex << static_cast<int>(list->buf[count]) << dec << " ";
        }
        cout << endl;
        list = list->next_mb; // Перемещаемся по списку
    }
}

// Печатаем названия файлов в консоль
void print_mem_file_img_t(mem_file_img_t* list) {
    if (!list) {
        cout << "Нет" << endl;
    }

    unsigned int index = 1;

    while (list) {
        cout << "Файл № " << index++ << ": " << list->path << ";" << endl;
        list = list->next_mfi;
    }
}

// Заполняем блоки памяти данными из файлов
return_mem readFiles(mem_block_t* block, mem_file_img_t* file, mem_block_t* block_full, mem_file_img_t* file_loaded) {
    
    if (!block || !file) { // Проверяем есть ли блоки памяти и файлы для чтения
        cerr << "Block or file NULL" << endl;
        return return_mem{ file, file_loaded, block, block_full };
    }

    LPDWORD read_symbols = 0; // Количество прочитанных символов из файла
    uint8_t* curr_block_buf = block->buf; // Заполняемый бит буфера
    size_t curr_block_size = block->mb_size; // Сколько места в буфере
    ULARGE_INTEGER curr_file_size; // Сколько бит осталось в файле
    HANDLE curr_file = NULL; // Текущий файл

    while (true) { // Цикл заполнения блоков памяти

        while (file) { // Пока не конец списка ищем файл доступный для чтения
            curr_file = CreateFileA(file->path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); // Открываем файл

            if (curr_file != INVALID_HANDLE_VALUE) { // Если файл открыт
                curr_file_size.LowPart = GetFileSize(curr_file, &curr_file_size.HighPart); // Запоминаем размер файла

                // Обеспечиваем потокобезопасность работы с файлом
                if (LockFile(curr_file, NULL, NULL, curr_file_size.LowPart, curr_file_size.HighPart)) { // Если файл заблокирован
                    break; // Выходим из цикла поиска доступного для чтения файла и переходим к заполнению блоков памяти
                }
                else {
                    CloseHandle(curr_file); // Закрываем файл
                }
            }

            file = file->next_mfi; // Перемещаемся к следующему файлу
        }

        if (!file) {
            return return_mem{ file, file_loaded, block, block_full };
        }

        while (true) {
            size_t size_read = curr_block_size < curr_file_size.QuadPart ? curr_block_size : curr_file_size.QuadPart; // Определяем, сколько байт считываем из файла

            ReadFile(curr_file, curr_block_buf, size_read, read_symbols, NULL); // Считываем данные в блок памяти

            curr_block_buf += size_read; // Смещаем указатель на заполнемый бит буфера
            curr_block_size -= size_read; // Уменьшаем количество свободного места в блоке памяти
            curr_file_size.QuadPart -= size_read; // Уменьшаем размер полезных данных в файле

            if (curr_file_size.QuadPart == 0) { // Если файл считался полностью

                if (!file_loaded) { // Если списка загруженных файлов нет
                    file_loaded = copy_mem_file_img_t(file);
                }
                else {
                    mem_file_img_t* temp = file_loaded; // Временный указатель для перемещения по списку

                    while (temp->next_mfi) { // Пока не встречаем конец
                        temp = temp->next_mfi; // Перемещаемся по списку
                    }
                    temp->next_mfi = copy_mem_file_img_t(file);
                }

                UnlockFile(curr_file, 0, 0, curr_file_size.LowPart, curr_file_size.HighPart); // Снимаем блокировку с файла

                CloseHandle(curr_file); // Закрываем файл
                file = file->next_mfi; // Переходим к следующему файлу

                if (!file) { // Если файлы закончились
                    return return_mem{ file, file_loaded, block, block_full }; // Выходим из функции
                }

                break; // Переходим к попытке открыть файл
            }

            if (curr_block_size == 0) { // Если блок памяти заполнен
                if (!block_full) { // Если заполненного блока нет
                    block_full = copy_mem_block_t(block);
                }
                else {
                    mem_block_t* temp = block_full; // Временный указатель для перемещения по списку

                    while (temp->next_mb) { // Пока не встречаем конец
                        temp = temp->next_mb; // Перемещаемся по списку
                    }
                    temp->next_mb = copy_mem_block_t(block);
                }

                block = block->next_mb; // Перемещаемся к следующему блоку

                if (!block) { // Если блоки памяти закончились
                    return return_mem{ file, file_loaded, block, block_full }; // Выходим из функции
                }

                curr_block_buf = block->buf; // Обновляем указатель на текущий заполняемый бит
                curr_block_size = block->mb_size; // Обновляем размер буфера
            }
        }
    }
}


int main()
{
    // Русскоязычная локализация
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    cout << "Сколько блоков памяти используем: ";
    unsigned int numbers_block;
    do {
        cin >> numbers_block;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(32767, '\n');
            cerr << "Неверное значение, введите еще раз сколько блоков памяти используем: ";
        }
        else {
            break;
        }
    } while (true);

    cout << "Размер блока памяти: ";
    size_t size_block;
    do {
        cin >> size_block;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(32767, '\n');
            cerr << "Неверное значение, введите еще раз размер блока памяти: ";
        }
        else {
            break;
        }
    } while (true);

    mem_block_t* mem_block_t_clear = NULL; // Объявляем блок памяти
    mem_block_t* mem_block_t_full = NULL; // Объявляем блок памяти для хранения заполненных
    while (numbers_block--) { // Заполняем блок(и) памяти
        mem_block_t_clear = add_mem_block_t(mem_block_t_clear, size_block);
    }

    cout << "\nБлоки памяти до записи:" << endl;
    print_mem_block_t(mem_block_t_clear);

    WIN32_FIND_DATAA file_struct; // Структура файла
    mem_file_img_t* mem_file_img_t_not_loaded = NULL; // Объявляем список файлов
    mem_file_img_t* mem_file_img_t_loaded = NULL; // Объявляем список файлов для хранения загруженных

    HANDLE h_file_struct = FindFirstFileA("*.txt", &file_struct); // Поиск файла
    
    cout << endl;
    if (h_file_struct != INVALID_HANDLE_VALUE) { // Если файл найден
        do {
            if((file_struct.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0){ // Если это не дирректория
                mem_file_img_t_not_loaded = add_file_img_t(mem_file_img_t_not_loaded, (char*)file_struct.cFileName); // Заполняем список файлов
                cout << "Добавлен файл " << file_struct.cFileName << endl;
            }
        } while (FindNextFileA(h_file_struct, &file_struct)); // Продолжаем поиск файлов
        FindClose(h_file_struct); // Закрываем поиск
    }

    return_mem result_ptr = readFiles(mem_block_t_clear, mem_file_img_t_not_loaded, mem_block_t_full, mem_file_img_t_loaded); // Заполняем блоки памяти

    mem_block_t_clear = result_ptr.block_clear;
    mem_block_t_full = result_ptr.block_full;
    mem_file_img_t_not_loaded = result_ptr.file_not_loaded;
    mem_file_img_t_loaded = result_ptr.file_loaded;

    cout << "\nБлоки памяти после записи:" << endl;
    print_mem_block_t(mem_block_t_full);

    cout << "\nОставшиеся блоки памяти:" << endl;
    print_mem_block_t(mem_block_t_clear);

    cout << "\nЗагруженные файлы:" << endl;
    print_mem_file_img_t(mem_file_img_t_loaded);

    cout << "\nНе загруженные файлы:" << endl;
    print_mem_file_img_t(mem_file_img_t_not_loaded);

    cout << "\nPress any button to the end...";
    cin.clear();
    cin.ignore(32767, '\n');
    cin.get();

    return 0;
}