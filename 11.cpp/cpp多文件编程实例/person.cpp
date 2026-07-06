#include "person.h"
using namespace std;

Person::Person(string name, int age, double height, double weight)
{   
    this->m_name = name;
    this->m_age = age;
    this->m_height = height;
    this->m_weight = weight;
    this->m_bmi = weight / (height * height);
}

string Person::getName()
{
    return this->m_name;
}

double Person::getBMI()
{
    return this->m_bmi;
}

void Person::print_person_info()
{
    cout << "Name: " << this->m_name << endl;
    cout << "Age: " << this->m_age << endl;
    cout << "Height: " << this->m_height << endl;
    cout << "Weight: " << this->m_weight << endl;
    cout << "BMI: " << this->m_bmi << endl;
}

Person::~Person()
{
    cout << "Person object destroyed." << endl;
}