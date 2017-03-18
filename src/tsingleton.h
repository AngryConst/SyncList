#ifndef TSINGLETON_H
#define TSINGLETON_H

/**
 * \brief Класс, позволяющий создать только один свой экземпляр.
 *
 * Пользователь не должен непосредственно создавать экземпляры класса, а должен
 * использовать статическую функцию \c instance(), которая возвращает ссылку на
 * созданный экземпляр. Экземпляр класса создается при первом вызове этой функции
 * и не уничтожается до конца выполнения Программы.
 *
 * Пример использования приведен ниже:
\code
	tMyClass: public tSingleton<tMyClass>
	{
		public:
			void foo();
	 ...
	};
	...
	tMyClass::instance().foo();
	...
\endcode
 *
 * \note Копирование объектов этого типа явно запрещено.
 * \note Класс можно использовать непосредственно, без указания пространства имен.
 *
 */
template< typename T > class tSingleton
{
public:
	//! Конструктор по-умолчанию.
	tSingleton() = default;

	//! Запрещенный конструктор копирования.
	tSingleton( const tSingleton& ) = delete;

	//! Запрещенный оператор присваивания.
	tSingleton &operator=( const tSingleton& ) = delete;

	//! Возвращает ссылку на единственный экземпляр объекта.
	static T &instance()
	{
		static T t;
		return t;
	}
};

#endif // TSINGLETON_H
