# foreachStructMember
If you need some reflection in C++ but Boost.PFR is too much of a heavy dependency, this is a single short header that you can easily copypaste anywhere.

## Usage
If a struct is aggregate initialisable, the `foreachStructMember` allows calling a generic lambda on each of its member variables. The labda will be instantiated to the right type for each member variable. If there are any parent classes, the lambda will be called on them before being called on member variables.

```C++
struct TestStruct {
	int a = 3;
	uint16_t b = 2;
	char c = 'c';
	int64_t d = 853925892038;
	std::string e = "This really works";
	double f = 1.43;
	float g = 2.716;
};

TestStruct test = {};
foreachStructMember(test, [] (auto& value) {
	std::cout << typeid(value).name() << " is " << value << std::endl;
});
```
An aggregate initialisable class has no private or protected member variables, no virtual methods, no base classes with virtual methods and no user declared constructors. If the conditions are not met, a static assert should fail with a suggestion to check if it is aggregate initialisable.

Here is a [link](https://godbolt.org/z/4zvGEedeK) to Compiler Explorer.

## Limitations
* Doesn't allow learning the names of the member variables (Boost.PFR allows that with C++20)
* Requires C++17 (doable in C++14, but the code would be longer)
* Works only with GCC and Clang (the principle works with MSVC, but I wasn't able to get this code to work there)
