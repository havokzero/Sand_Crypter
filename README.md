Sand Crypter

![Sand Crypter GUI](https://cdn.discordapp.com/attachments/1080373543625293844/1257542828637159476/image.png?ex=6684c986&is=66837806&hm=473a8fbcfbfe8e344d96575377a76775f264486fb87e78a0e0b85a2c8481cd88&)

Sand Crypter is a Windows application designed for encrypting and bundling two files together. The primary file remains executable, while the secondary (hidden) file is encrypted and executed from memory when the primary file runs. This project is intended for educational and research purposes in the field of cybersecurity.

Sand Crypter GUI
Features

    Drag and Drop Interface: Easily select files by dragging them into the designated areas.
    Randomized Key and IV: Each encryption operation generates a new random key and IV for enhanced security.
    XOR Obfuscation: Additional layer of XOR encryption to obfuscate the encrypted data.
    Icon Retention: The bundled executable retains the icon of the primary file unless a custom icon is specified.

Prerequisites

    Windows OS
    Visual Studio (for building the project)
    OpenSSL (for encryption)

Installation

    Clone the repository:

    bash

    git clone https://github.com/havokzero/Sand_Crypter.git
    cd Sand_Crypter

    Open the project in Visual Studio:
        Launch Visual Studio and open the solution file Sand_Crypter.sln.

    Install OpenSSL:
        Download and install OpenSSL from https://slproweb.com/products/Win32OpenSSL.html.
        Ensure the OpenSSL binaries are in your system's PATH.

    Build the project:
        Set the build configuration to Release or Debug.
        Build the solution by selecting Build > Build Solution or pressing Ctrl+Shift+B.

Usage

    Run Sand Crypter:
        Execute the compiled Sand_Crypter.exe.

    Select Files:
        Drag the primary file into the "Drop Main File Here or Click to Select" area.
        Drag the hidden file into the "Drop Hidden File Here or Click to Select" area.

    Select Output Directory:
        Click the "Select Output Directory" button to choose where the bundled executable will be saved.

    Optional: Select Custom Icon:
        Click the "Select Icon" button to specify a custom icon for the bundled executable.

    Encrypt and Bundle:
        Click the "Melt" button to encrypt the hidden file and bundle it with the primary file.

How It Works
Encryption Process

    Random Key and IV Generation:
        A random key and IV are generated using OpenSSL's RAND_bytes.

    File Encryption:
        The hidden file is encrypted using AES-256-CBC with the generated key and IV.

    XOR Obfuscation:
        The encrypted data is further obfuscated using a randomly generated XOR key.

    Bundling:
        The encrypted data is bundled with the primary file. The resulting executable retains the functionality of the primary file and executes the hidden file from memory.

Disclaimer

This project is intended for educational and research purposes only. Use it responsibly and only on files you own or have explicit permission to use. Misuse of this tool may violate laws and regulations.
Contributing

Contributions are welcome! Please open an issue or submit a pull request if you have any improvements or bug fixes.
License

This project is licensed under the MIT License. See the LICENSE file for details.
