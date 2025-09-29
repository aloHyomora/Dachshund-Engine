#include <iostream>
#include "calculator.h"
using namespace std;
        

int main(){
    Calculator calc;
    cout << "Hello, Dachshund Engine!" << endl;
    cout << "5 + 3 = " << calc.add(5, 3) << endl;
    cout << "5 - 3 = " << calc.subtract(5, 3) << endl;
    cout << "5 * 3 = " << calc.multiply(5, 3) << endl;
    cout << "5 / 3 = " << calc.divide(5, 3) << endl;
    cout << "This is Dachshund Engine!" << endl;
    return 0;
}