// autor:       Jakub Ostrzołek
// opis:        zadanie dodatkowe ZPR - C++20 coroutines
// kompilacja:  g++ -fcoroutines coroutines.cpp

#include <coroutine>
#include <iostream>

// Coroutine to koncepcja znana z innych języków, takich jak JavaScript, Kotlin,
// Go i innych. Jest to funkcja, która potrafi zapamiętać swój stan na stercie i
// zawiesić się w trakcie wykonywania, dopóki nie zostanie ponownie wznowiona
// przez kod wywołujący taką funkcję.
//
// Implementacje w innych językach są zazwyczaj łatwiejsze w użyciu ze względu
// na obecność garbage collectora. W C++ nie ma takiego mechanizmu i trzeba
// zadbać o poprawne czyszczenie wcześniej wspomnianego obiektu na stercie.
//
// Coroutines C++20 nie są trywialne w użyciu. Aby zdefiniować coroutine należy
// zdefiniować funkcję spełniającą 2 nadrzędne warunki:
//  * użyć jednego z nowo wprowadzonych operatorów: co_await, co_yield lub
//  co_return,
//  * posiadać typ zwracany zgodny z pewnymi warunkami

// 1. Prosty przykład

namespace p1 {
struct promise;

// Wartość zwracana przez coroutine.
//
// Powinna (choć formalnie nie musi) agregować lub dziedziczyć po std::coroutine_handle<promise>,
// aby można było z kodu wywołującego tę coroutine wznowić jej wykonywanie za pomocą
// std::coroutine_handle<...>::operator().
struct coroutine : std::coroutine_handle<promise> {
  // Musi posiadać podklasę promise_type.
  using promise_type = p1::promise;
};

// "Obietnica" - obiekt tworzony przy starcie coroutine. W głównej mierze decyduje on o zachowaniu
// coroutine w różnych przypadkach.
struct promise {
  // Wymagana funkcja zamiany obietnicy na std::coroutine_handle<promise> (ponieważ coroutine
  // dziedziczy po std::coroutine_handle<promise>, to możemy użyć coroutine). Raczej w każdym
  // przypadku będzie wyglądać podobnie.
  coroutine get_return_object() { return {coroutine::from_promise(*this)}; }

  // Definiuje zachowanie coroutine zaraz po stworzeniu. W zależności od klasy wartości zwracanej:
  // * std::suspend_always => zawiesimy coroutine zaraz po stworzeniu (działanie 'leniwe'),
  // * std::suspend_never => będziemy wykonywać kod coroutine aż do następnego zawieszenia
  //   (działanie 'gorliwe').
  //
  // std::suspend_always i std::suspend_never to specjalne klasy tzw. 'awaiterów' (lub 'awaitable'),
  // czyli klas, instancji których możemy użyć w wyrażeniu po operatorze co_await. Można tworzyć
  // własne klasy 'awaiterów' w celu optymalizacji kodu lub specjalnego zachowania, jednak nie jest
  // to rozwinięte w moich przykładach.
  std::suspend_always initial_suspend() noexcept { return {}; }

  // Definiuje zachowanie coroutine zaraz po zakończeniu działania, czyli w jednej z sytuacji:
  // 1) wywołanie co_return `expr` - kończymy działanie i zwracamy wartość,
  // 2) wywołanie co_return - kończymy działanie i nie zwracamy wartość,
  // 3) 'wypadnięcie poza funkcję' - kończymy działanie i nie zwracamy wartości.
  //
  // W zależności od klasy zwracanej:
  // * std::suspend_always => zawiesimy coroutine zaraz po zakończeniu działania. Oznacza to, że
  //   obiekt reprezentujący stan coroutine na stercie zostanie w całości i będzie można z niego
  //   jeszcze odczytać wartości, natomiast trzeba będzie usunąć obiekt ze sterty później.
  // * std::suspend_never => zaraz po zakończeniu działania usuniemy obiekt ze sterty.
  //
  // W przypadku 1) zostanie wywołana dodatkowo metoda promise::return_value(...), a w przypadkach
  // 2) i 3) metoda promise::return_void(...).
  //
  // UWAGA: w przypadku 1) i 2) jeżeli nie dostarczymy definicji odpowiednich funkcji
  // promise::return_* program się nie skompiluje, natomiast w przypadku 3) będzie to
  // niezdefiniowane zachowanie!
  std::suspend_always final_suspend() noexcept { return {}; }
  // Zobacz -> opis promise::final_suspend() wyżej.
  //
  // Może być zdefiniowana tylko jedna metoda z promise::return_void() lub
  // promise::return_value(...).
  void return_void() {}
  //void return_value(...) {}

  // Funkcja wywoływana w przypadku wystąpienia wyjątku w coroutine. Funkcja powinna wydostać
  // wyjątek za pomocą std::current_exception() i zapisać ją w promise. W tym przykładzie jest to
  // pominięte, przez co wszelkie wyjątki wewnątrz coroutine zostaną cicho zignorowane.
  void unhandled_exception() {}
};

// Coroutine, która wypisuje kolejną liczbę na ekran za każdym wznowieniem
coroutine counter() {
  for (std::size_t i = 0;; i++) {
    std::cout << "coroutine: " << i << std::endl;
    co_await std::suspend_always();
  }
}

auto main() -> void {
  auto h = counter();
  for (std::size_t i = 0; i < 3; i++) {
    h();
  }
  return;
}
} // namespace p1

auto main() -> int {
  std::cout << "<--- p1 --->" << std::endl;
  p1::main();
  return 0;
}

// https://en.cppreference.com/w/cpp/language/coroutines
// https://www.scs.stanford.edu/~dm/blog/c++-coroutines.html
