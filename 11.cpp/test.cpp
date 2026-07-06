#include <iostream>
#include <string>
#include <stdexcept>
using namespace std;

template <typename T>
class general_arr {
private:
    T* arr;
    int size;
public:
    general_arr(int s) {
        if (s <= 0) {
            throw std::invalid_argument("Size must be positive");
        }
        size = s;
        arr = new T[size];
    }

    general_arr(const general_arr& other) {
        size = other.size;
        arr = new T[size];
        for (int i = 0; i < size; i++) {
            arr[i] = other.arr[i];
        }
    }

    general_arr& operator=(const general_arr& other) {
        if (this != &other) {
            delete[] arr;
            size = other.size;
            arr = new T[size];
            for (int i = 0; i < size; i++) {
                arr[i] = other.arr[i];
            }
        }
        return *this;
    }

    ~general_arr() {
        delete[] arr;
    }

    T &operator[](int index);
    T min();
};

template <typename T>
T &general_arr<T>::operator[](int index) {
    if (index >= size || index < 0) {
        throw std::out_of_range("Index out of bounds");
    }
    return arr[index];
}


template <typename T>
T general_arr<T>::min(){
    if (size == 0) {
        throw std::out_of_range("Array is empty");
    }
    T min_val = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] < min_val) {
            min_val = arr[i];
        }
    }
    return min_val;
}

int main() { 
    try {
        general_arr<float> S(4);
        S[0] = 5.2;
        S[1] = 5.1;
        S[2] = 5;
        S[3] = 4.9;
        // S[4] = "?"; // 移除越界测试
        cout << S.min() << endl;
    } catch (const std::exception& e) {
        cout << "Error: " << e.what() << endl;
    }

    return 0;
}
