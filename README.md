This is a Ghost password manager designed to be secure, obscure and free

what this is meant to do:

provide anti brute force mechanisms (right now only for script kiddies)

provide industry standard encryption (AES-256, chachapoly)

wipe itself from system and memory (including leakage to hard drive and .exe task manager logs/terminal past logs)

fit into 1 USB drive of atleast 2gb of space

provide high security and featuring multi layer encrypt

outer password > (decrypting the double encrypted layers) > master password > (decrypt the second layer) > "individual master password for each vault" > (decrypt the passwords and vaults)
altho ofc this can be optional and is for more paranoid users


it also has TCATO and ATTEMPTS to capture exit events liek ctrl + C or window close

it has support for both linux and windows, still working on translation to other lenguages which ill probably just do in platform and make it request throught another file called translate.cpp which will return the fixed translations, prb will use google translate cuz why not

this is complelty offline and has yet to have a way to connect to any databse, idk nothing about networking soooo...

ill implement security on startup.cpp so it checks if its safe to run or not then shut down,
encryption.cpp is lwk complex I will add comments lol



this app is made by XxG4ToTxX and its lwk tuff
