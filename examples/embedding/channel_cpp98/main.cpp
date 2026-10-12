/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/channel_cpp98/main.cpp.
 */

/* C++98 example: RAII wrapper over the stable C89 channel API. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/channel_value.h"
/* Подключаем объявления используемых типов и функций. */
#include <cassert>
/* Define ChannelOwner class and its representation. */
/* Объявляем тип, используемый в этом примере. */
class ChannelOwner
/* Начинаем блок инструкций или описание объекта. */
{
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_ChannelValue value_;
    /* Initialize Channel Owner and its owned state. */
    /* Выполняем вызов ChannelOwner с указанными аргументами. */
    ChannelOwner(const ChannelOwner &);
    /* Сохраняем вычисленное значение: ChannelOwner &operator. */
    ChannelOwner &operator=(const ChannelOwner &);

    /* Точка перехода для общей ветки обработки и очистки. */
  public:
    /* Initialize Channel Owner and its owned state. */
    /* Выполняем вызов ChannelOwner с указанными аргументами. */
    ChannelOwner()
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Сохраняем вычисленное значение: value_.channel. */
        value_.channel = 0;
        /* Выполняем вызов assert с указанными аргументами. */
        assert(fvm_channel_value_create(&value_) == FVM_OK);
        /* Завершаем текущий блок или описание объекта. */
    }
    /* Release the resources owned by this ChannelOwner instance. */
    /* Выполняем вызов ChannelOwner с указанными аргументами. */
    ~ChannelOwner()
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Освобождаем одну удержанную ссылку на канал. */
        fvm_channel_value_release(&value_);
        /* Завершаем текущий блок или описание объекта. */
    }
    /* Read the current value. */
    /* Выполняем вызов get с указанными аргументами. */
    FVM_ChannelValue *get()
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return &value_;
        /* Завершаем текущий блок или описание объекта. */
    }
    /* Завершаем текущий блок или описание объекта. */
};

/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main()
/* Начинаем блок инструкций или описание объекта. */
{

    /* Initialization / Инициализация локального состояния. */
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    ChannelOwner owner;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_ChannelValue other;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value input, output;
    /* Удерживаем ещё одну ссылку на общий канал без копирования очереди. Проверяем успешность
     * вызова. */
    assert(fvm_channel_value_copy(&other, owner.get()) == FVM_OK);
    /* Записываем логическое значение и соответствующий тег типа. */
    fvm_value_boolean(&input, 1);

    /* Output / Вывод и передача результата. */
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_channel_value_send(&other, &input) == FVM_OK);

    /* Computation / Вычисление и обновление состояния. */
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_channel_value_receive(owner.get(), &output) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(output.type == FVM_VALUE_BOOLEAN && output.boolean);
    /* Освобождаем одну удержанную ссылку на канал. */
    fvm_channel_value_release(&other);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
