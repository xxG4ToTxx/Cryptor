This is a Ghost password manager designed to be secure, obscure and free

what this is meant to do:

provide anti brute force mechanisms (right now only for script kiddies)

use Argon2id for key derivation and XChaCha20-Poly1305 authenticated encryption

wipe itself from system and memory (including leakage to hard drive and .exe task manager logs/terminal past logs)

fit into 1 USB drive of atleast 2gb of space

provide high security and featuring multi layer encrypt

outer password > (decrypting the double encrypted layers) > master password > (decrypt the second layer) > "individual master password for each vault" > (decrypt the passwords and vaults)
altho ofc this can be optional and is for more paranoid users


TACTO means Two-Channel Auto-Type Obfuscation. When enabled, a saved password
can be auto-typed by pasting its first half from the clipboard and sending the
remaining half as simulated keystrokes. The clipboard is cleared afterward.
TACTO requires Windows or Linux X11; Linux Wayland is not supported.
Linux builds require X11 development files, and TACTO additionally requires
the X11 XTest runtime library.

TACTO is an input convenience, not protection against malware, keyloggers, or
clipboard monitoring. Auto-type supports printable ASCII passwords.

Temporary artifact cleanup and handled exit signals are separate features.

it has support for both linux and windows, still working on translation to other lenguages which ill probably just do in platform and make it request throught another file called translate.cpp which will return the fixed translations, prb will use google translate cuz why not

this is complelty offline and has yet to have a way to connect to any databse, idk nothing about networking soooo...

ill implement security on startup.cpp so it checks if its safe to run or not then shut down,
The encryption implementation uses Argon2id and XChaCha20-Poly1305.



this app is made by XxG4ToTxX and its lwk tuff
