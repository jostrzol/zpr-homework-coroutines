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

// 1. Prosty przykład - licznik wypisujący na ekran kolejne liczby.
namespace p1 {
struct promise;

// Wartość zwracana przez coroutine.
//
// Powinna (choć formalnie nie musi) agregować lub dziedziczyć po
// std::coroutine_handle<promise>, aby można było z kodu wywołującego tę
// coroutine wznowić jej wykonywanie za pomocą
// std::coroutine_handle<...>::operator().
struct coroutine : std::coroutine_handle<promise> {
  // Musi posiadać podklasę promise_type.
  using promise_type = p1::promise;
};

// "Obietnica" - obiekt tworzony przy starcie coroutine. W głównej mierze
// decyduje on o zachowaniu coroutine w różnych przypadkach.
struct promise {
  // Wymagana funkcja zamiany obietnicy na coroutine. Na początku życia
  // coroutine tworzy obiekt promise, który następnie musi zostać zamieniony w
  // obiekt coroutine w celu przekazania do kodu wołającego. Do tego właśnie
  // wykorzystywana jest metoda promise::get_return_object(). Raczej w każdym
  // przypadku będzie wyglądać podobnie.
  coroutine get_return_object() {
    // Metoda std::coroutine_handle<...>::from_promise() korzysta z tego, że w
    // obiekcie stanu coroutine na stosie jest stałe przesunięcie między
    // obiektem promise a coroutine_handle (oba są tworzone przy wejściu do
    // coroutine -> zobacz komentarze do f. counter() niżej)
    return {coroutine::from_promise(*this)};
  }

  // Definiuje zachowanie coroutine zaraz po stworzeniu. W zależności od klasy
  // wartości zwracanej:
  // * std::suspend_always => zawiesimy coroutine zaraz po stworzeniu (działanie
  // 'leniwe'),
  // * std::suspend_never => będziemy wykonywać kod coroutine aż do następnego
  // zawieszenia
  //   (działanie 'gorliwe').
  //
  // std::suspend_always i std::suspend_never to specjalne klasy tzw.
  // 'awaiterów' (lub 'awaitable'), czyli klas, instancji których możemy użyć w
  // wyrażeniu po operatorze co_await. Można tworzyć własne klasy 'awaiterów' w
  // celu optymalizacji kodu lub specjalnego zachowania, jednak nie jest to
  // rozwinięte w moich przykładach.
  std::suspend_always initial_suspend() noexcept { return {}; }

  // Definiuje zachowanie coroutine zaraz po zakończeniu działania, czyli w
  // jednej z sytuacji: 1) wywołanie co_return `expr` - kończymy działanie i
  // zwracamy wartość, 2) wywołanie co_return - kończymy działanie i nie
  // zwracamy wartość, 3) 'wypadnięcie poza funkcję' - kończymy działanie i nie
  // zwracamy wartości.
  //
  // W zależności od klasy zwracanej:
  // * std::suspend_always => zawiesimy coroutine zaraz po zakończeniu
  // działania. Oznacza to, że
  //   obiekt reprezentujący stan coroutine na stercie zostanie w całości i
  //   będzie można z niego jeszcze odczytać wartości, natomiast trzeba będzie
  //   usunąć obiekt ze sterty później.
  // * std::suspend_never => zaraz po zakończeniu działania usuniemy obiekt ze
  // sterty.
  //
  // W przypadku 1) zostanie wywołana dodatkowo metoda
  // promise::return_value(...), a w przypadkach 2) i 3) metoda
  // promise::return_void(...).
  //
  // UWAGA: w przypadku 1) i 2) jeżeli nie dostarczymy definicji odpowiednich
  // funkcji promise::return_* program się nie skompiluje, natomiast w przypadku
  // 3) będzie to niezdefiniowane zachowanie!
  std::suspend_always final_suspend() noexcept { return {}; }
  // Zobacz -> opis promise::final_suspend() wyżej.
  //
  // Może być zdefiniowana tylko jedna metoda z promise::return_void() lub
  // promise::return_value(...).
  void return_void() {}
  // void return_value(...) {}

  // Funkcja wywoływana w przypadku wystąpienia wyjątku w coroutine. Funkcja
  // powinna wydostać wyjątek za pomocą std::current_exception() i zapisać ją w
  // promise. W tym przykładzie jest to pominięte, przez co wszelkie wyjątki
  // wewnątrz coroutine zostaną cicho zignorowane.
  void unhandled_exception() {}
};

// Coroutine, która wypisuje kolejną liczbę na ekran za każdym wznowieniem
coroutine counter() {
  // Na początku, przed kodem coroutine, zapisywany jest na stercie obiekt
  // reprezentujący stan funkcji, czyli m.in. wartości parametrów, miejsce
  // ostatniego przerwania, std::coroutine_handle<coroutine::promise_type> i
  // instancję coroutine::promise_type (tutaj promise).
  //
  // Następnie wołany jest `co_await promise::initial_suspend()`, czyli w
  // zależności od wartości zwracanej initial_suspend albo zawiesimy coroutine
  // albo nie.
  for (std::size_t i = 0;; i++) {
    std::cout << "coroutine: " << i << std::endl;
    // Zawieszamy wykonanie coroutine.
    co_await std::suspend_always();
  }
  // Nigdy nie 'wypadamy' z funkcji, więc coroutine nigdy się nie kończy.
}

auto main() -> void {
  auto h = counter();
  for (std::size_t i = 0; i < 3; i++) {
    std::cout << "main: " << i << std::endl;
    h();
  }
  // Musimy ręcznie zniszczyć obiekt stanu coroutine ze sterty (później będzie
  // pokazane jak zrobić to automatycznie za pomocą wzorca RAII)
  h.destroy();
  return;
}
} // namespace p1

// 2. Przykład - licznik generujący kolejne liczby za pomocą co_yield oraz
// zwracający wartość przy zakończeniu za pomocą co_return.
namespace p2 {
struct promise;

struct coroutine : std::coroutine_handle<promise> {
  using promise_type = p2::promise;
};

struct promise {
  // Dodajemy wartość do klasy promise, którą coroutine może zapisywać przy
  // wywołaniu co_yield `expr`.
  std::size_t value;

  // Dodajemy wartość do klasy promise, którą coroutine może zapisywać przy
  // wywołaniu co_return `expr`.
  std::string ret;

  coroutine get_return_object() { return {coroutine::from_promise(*this)}; }

  // Zmieniono na suspend_never tak, aby pętla w main mogła od razu przeczytać
  // wartość.
  std::suspend_never initial_suspend() noexcept { return {}; }
  std::suspend_always final_suspend() noexcept { return {}; }

  // Zamieniamy return_void() na return_value(), aby móc zapisać wartość do pola
  // ret przy zakończeniu coroutine.
  void return_value(std::string &&ret_value) { ret = ret_value; }

  // Ta funkcja będzie wołana przy wywołaniu operatora co_yield `expr`.
  //
  // Tutaj zapisujemy przekazaną wartość w polu value, a następnie zwracamy
  // obiekt std::suspend_always, dzięki czemu coroutine się zawiesi każdorazaowo
  // po wywpołaniu co_yield `expr`. Oczywiście możemy zwracać własny 'awaiter' w
  // celu optymalizacji (być może coroutine wcale nie musi się od razu zawiesić,
  // a policzyć przyszłe wartości, co oszczędziłoby każdorazowego, kosztownego
  // zapisywania stanu funkcji do sterty).
  std::suspend_always yield_value(std::size_t i) {
    value = i;
    return {};
  }
  void unhandled_exception() {}
};

// Coroutine, zapisująca do obiektu promise kolejne liczby, które może odczytać
// funkcja wywołująca.
coroutine counter(std::size_t max) {
  for (std::size_t i = 0; i < max; i++) {
    // co_yield `expr` może być koncepcyjnie zamienione na: co_await
    // promise.yield_value(`expr`)
    std::cout << "coroutine: generated: " << i << std::endl;
    co_yield i;
  }
  std::cout << "coroutine: ending" << std::endl;
  co_return "maximum value reached";
}

auto main() -> void {
  auto h = counter(3);
  auto &p = h.promise();
  while (!h.done()) {
    // Zamieniliśmy kolejność, najpierw wypisujemy, potem wznawiamy coroutine.
    // Możemy tak zrobić, ponieważ teraz promise::initial_suspend zwraca
    // std::suspend_never i wartość jest wyliczona przy tworzeniu coroutine.
    std::cout << "main: got from coroutine: " << p.value << std::endl;
    h();
  }
  // Tutaj coroutine musiała zakończyć działanie, bo h.done() == true. Zatem
  // możemy przeczytać wartość zwróconą.
  std::cout << "main: coroutine endend: " << p.ret << std::endl;

  h.destroy();
  return;
}
} // namespace p2

auto main() -> int {
  std::cout << "<--- p1 --->" << std::endl;
  p1::main();
  std::cout << "<--- p2 --->" << std::endl;
  p2::main();
  return 0;
}

// https://en.cppreference.com/w/cpp/language/coroutines
// https://www.scs.stanford.edu/~dm/blog/c++-coroutines.html
