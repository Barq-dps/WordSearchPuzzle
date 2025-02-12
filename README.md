Here is a **README.md** file for your project, including installation instructions for **HTTPLIB, SFML, and ImGUI**.

---

## **🔠 Word Search Solver - C++ Game**  
A fun **Word Search Game** built in **C++**, featuring **random word generation, grid placement, and interactive gameplay**.  
The game dynamically fetches words from an **online API** and places them into a grid.  

---

## **📌 Features**
✔ **Dynamic Word Fetching**: Uses **HTTPLIB** to retrieve words from an API.  
✔ **Multi-Difficulty Levels**: Choose from **Baby, Adult, and Expert modes**.  
✔ **Optimized Grid Placement**: Words are placed first, then the grid is filled.  
✔ **Interactive UI with SFML & ImGUI**: A simple graphical interface for better gameplay.  
✔ **Fast Performance**: Uses **multithreading** for word validation and fetching.  

---

## **📥 Installation Guide**
### **🔹 Prerequisites**
Before running the project, make sure you have:
- **Visual Studio (2022 or later)**
- **CMake (for building dependencies)**
- **vcpkg** (for package management)

---

## **📌 Step 1: Install Dependencies**
### **1️⃣ Install HTTPLIB**
HTTPLIB is used for making HTTP requests to fetch words.  

🔹 **Using vcpkg** (Recommended):
```sh
vcpkg install httplib:x64-windows
```
🔹 **Manual Installation**:
1. Download HTTPLIB: [HTTPLIB GitHub](https://github.com/yhirose/cpp-httplib)  
2. Copy **httplib.h** to your project’s `external/httplib` folder.  
3. Add `#define CPPHTTPLIB_OPENSSL_SUPPORT` to enable HTTPS support.

---

### **2️⃣ Install SFML**
SFML is used for rendering graphics in the game.

🔹 **Using vcpkg** (Recommended):
```sh
vcpkg install sfml:x64-windows
```
🔹 **Manual Installation**:
1. Download SFML: [SFML Official Site](https://www.sfml-dev.org/download.php)  
2. Extract it into `C:\SFML`  
3. In **Visual Studio**, go to:
   - `Project → Properties → C/C++ → General → Additional Include Directories`
   - Add:  
     ```
     C:\SFML\include
     ```
   - `Linker → General → Additional Library Directories`
   - Add:  
     ```
     C:\SFML\lib
     ```
   - `Linker → Input → Additional Dependencies`
   - Add:
     ```
     sfml-graphics.lib
     sfml-window.lib
     sfml-system.lib
     sfml-audio.lib
     ```

📢 **Copy DLLs**:  
Copy all **.dll** files from `C:\SFML\bin` into your `x64/Debug` folder.

---

### **3️⃣ Install ImGUI**
ImGUI is used for creating user interface elements.

🔹 **Using vcpkg**:
```sh
vcpkg install imgui:x64-windows
```
🔹 **Manual Installation**:
1. Download ImGUI: [ImGUI GitHub](https://github.com/ocornut/imgui)  
2. Add the **imgui folder** to your project (`external/imgui`).
3. Link it in **Visual Studio**:
   - `C/C++ → Additional Include Directories`: Add `external/imgui`
   - `Linker → Input → Additional Dependencies`: Add `imgui.lib`

---

## **🚀 Running the Game**
1. **Open the project in Visual Studio.**
2. **Select `x64 Debug` mode.**
3. **Build and Run (`Ctrl + F5`).**
4. **Choose a difficulty mode.**
5. **Find the words hidden in the grid!** 🧩

---
---

## **🔧 Troubleshooting**
### **❌ HTTPLIB Not Found**
✔ Ensure the **HTTPLIB header file** is correctly included.  
✔ If using `#define CPPHTTPLIB_OPENSSL_SUPPORT`, make sure OpenSSL is installed.

### **❌ SFML Missing DLLs**
✔ Copy SFML `.dll` files from `C:\SFML\bin` to your project’s `x64/Debug` folder.

### **❌ API Requests Not Working**
✔ Check your internet connection.  
✔ If the API is down, try a different word API.

---

## **📜 License**
This project is licensed under the **MIT License**.
