
//main.cpp
#include <iostream>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <thread>




#include "startup.h"
#include "ui.h"




bool exitLoop = false;
int count = 3;




int main(){
while(exitLoop == false && count > 0){
    system("cls");
    std::cout << "-_-_-_-CRYPTOR-_-_-_-\n      LOADING...";

std::string Startup_status = StartUp();


if(Startup_status == "Failed to initialize"){
    
    count--;

} else{ exitLoop = true; };


}
if(count == 0){
    return 1;
};

system("cls");
    std::cout << "-_-_-_-CRYPTOR-_-_-_-\n      Succesfully Initialized!";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    MainMenu(); //for ui.cpp

    ;



    



};