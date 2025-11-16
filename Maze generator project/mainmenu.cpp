#include <iostream>
using namespace std;
int main() {
    int choice = 0;

    
    while (true) {
        
        cout << "===================================" << endl;
        cout << "     СИМУЛАТОР НА ЛАБИРИНТ     " << endl;
        cout << "===================================" << endl;
        cout << "    1. Ниво 1 " << endl;
        cout << "    3. Ниво 3 " << endl;
        cout << "    2. Ниво 2 " << endl;
        cout << "    4. Ниво 4 (" << endl;
        cout << "    5. Изход" << endl;
        cout << "===================================" << endl;
        if (!(cin >> choice)) {
            cout << "\nНевалиден вход! Моля, въведете ЧИСЛО." << endl;
        }
        switch (choice) {
        case 1:
            break;
        case 2:
            break;
        case 3:

            break;
        case 4:
            break;
        case 5:
            cout << "Излизане от симулатора... Довиждане!" << endl;
        default:
            cout << "\nНевалиден избор. Моля, изберете число между 1 и 5." << endl;
            cout << "Натиснете Enter за да опитате отново...";
            cin.get();
            break;
        }
    }