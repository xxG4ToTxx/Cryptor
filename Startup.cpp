
//startup.cpp

#include <vector>
#include <string>
#include <iostream>
#include <sodium.h>

#include "VaultManager.h"





extern "C" {
    #include <sodium.h>
}


std::string StartUp(){
std::string VaultDirectoryStatus = InitializeVaultDirectory();


//std::string VaultStatus = LookForVaults();
//paused for developing

std::cout << VaultDirectoryStatus;
if(VaultDirectoryStatus == "PASSED CHECKS"){
       if (sodium_init() < 0) {
            system("cls");
        std::cerr << "Libsodium failed to initialize, try running the file again or check for updates...\n";
        //initialize libsodium
         }
         else{






return "SUCCESS";


         };
         

}
else if(VaultDirectoryStatus == "ERROR"){
system("cls");
return "Failed to initialize";


}
return "UNEXPECTED";


};






