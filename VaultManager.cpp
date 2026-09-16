
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

bool DirectoryExists = false;
bool isFolder = false;

//true for now for development... 

struct Vault{
    std::string name;
    int ID;
    std::string creation_date;
    std::string Path;
};



std::string CreateNewVault(std::string NextTask){

    std::string VaultName;
    system("cls");

    std::cout << "enter desired name of vault\n  > ";
    std::cin >> VaultName;
    
    fs::path targetFolder = "C:/Documents/cryptor";
    fs::path file_name(VaultName);
     fs::create_directories(targetFolder);
     fs::path full_path = targetFolder / file_name;






return "SUCCESS";

};



std::string LookForVaults(){

    fs::path targetFolder = "C:/Documents/cryptor";
for (const auto& entry : fs::directory_iterator(targetFolder)) {
};

    
return "good";
};


std::string InitializeVaultDirectory(){


    fs::path targetFolder = "C:/Documents/cryptor";

            if(!fs::exists(targetFolder)){
                    fs::create_directories(targetFolder);
                    DirectoryExists = true;
                    isFolder = true;
            }
            else if (!fs::is_directory(targetFolder)){
                std::cerr << "Fatal Error: folder location is a file...\n <> try deleting the file or restarting the application";
            } 
            else{

                isFolder = true;
                DirectoryExists = true;

            };



            if(DirectoryExists == true && isFolder == true){

                
    return "PASSED CHECKS";

            }
            else if(DirectoryExists == true && isFolder == false){

                return "ERROR";
            }
        

return "UNEXPECTED";
};
  


//C:/Documents/cryptor