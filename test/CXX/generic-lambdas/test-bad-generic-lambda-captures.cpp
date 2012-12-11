// RUN: %clang_cc1 -fsyntax-only -verify -std=c++1y %s


void test_capture_errors() {
  int local = 5; //expected-note {{'local' declared here}}
  auto L = [](auto a) //expected-note {{lambda expression begins here}}
            { return local; }; //expected-error {{variable 'local' cannot be implicitly captured}}
  
}
int main() {

}

