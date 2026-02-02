#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <deque>
#include <vector>
#include <algorithm>

// ========================================
// CLIPBOARD MANAGER - CONFIGURABLE VERSION
// ========================================

const std::string SAVE_FILE = "clipboard_slots.dat";

// ID for system tray icon
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 2001
#define ID_TRAY_ABOUT 2002
#define ID_TRAY_TOGGLE_CONSOLE 2003

// Global variables
NOTIFYICONDATA nid;
HWND g_hwnd = NULL;
HWND g_console = NULL;
HHOOK g_hook = NULL;
bool g_running = true;
bool g_consoleVisible = true;

// Configurable keys (default values)
int KEY_SAVE1 = 0xBA;      // $ by default
int KEY_SAVE2 = 0xDD;      // £ by default
int KEY_LOAD = 0xDE;       // ² by default
int KEY_CLEAR = 0x43;      // C

// Characters for slot keys (configurable)
std::string SLOT_CHARS[10] = {"&", "é", "\"", "'", "(", "-", "è", "_", "ç", "à"};

// AZERTY slot keys (fixed VK codes)
int slotKeys[] = {
    0x31, // 1 or & (index 0)
    0x32, // 2 or é (index 1)
    0x33, // 3 or " (index 2)
    0x34, // 4 or ' (index 3)
    0x35, // 5 or ( (index 4)
    0x36, // 6 or - (index 5)
    0x37, // 7 or è (index 6)
    0x38, // 8 or _ (index 7)
    0x39, // 9 or ç (index 8)
    0x30  // 0 or à (index 9)
};

// Key states
std::map<int, bool> keyPressed;
std::map<int, bool> actionExecuted;

// Action history (max 4)
std::deque<std::string> actionHistory;

// Slot number accumulation
std::string currentSlotNumber = "";
bool isAccumulatingSlot = false;
bool isClearMode = false;  // CLEAR mode active with LOAD+C

// ========================================
// HISTORY MANAGEMENT
// ========================================

void addToHistory(const std::string& action) {
    actionHistory.push_front(action);
    if (actionHistory.size() > 4) {
        actionHistory.pop_back();
    }
}

void displayHistory() {
    if (actionHistory.empty()) {
        std::cout << "[No recent actions]\n" << std::endl;
    } else {
        for (const auto& action : actionHistory) {
            std::cout << action << std::endl;
        }
        std::cout << std::endl;
    }
}

// ========================================
// CONSOLE MANAGEMENT
// ========================================

void showConsole() {
    if (g_console) {
        ShowWindow(g_console, SW_SHOW);
        SetForegroundWindow(g_console);
        g_consoleVisible = true;
    }
}

void hideConsole() {
    if (g_console) {
        ShowWindow(g_console, SW_HIDE);
        g_consoleVisible = false;
    }
}

void toggleConsole() {
    if (g_consoleVisible) {
        hideConsole();
    } else {
        showConsole();
    }
}

// ========================================
// CONVERSION UTILITIES
// ========================================

int charToVK(char c) {
    // Convert a character to Windows VK code
    // For letters A-Z, it's simple: VK_A = 0x41, VK_B = 0x42, etc.
    c = toupper(c);
    if (c >= 'A' && c <= 'Z') {
        return 0x41 + (c - 'A');  // VK_A = 0x41
    }
    // For digits 0-9
    if (c >= '0' && c <= '9') {
        return 0x30 + (c - '0');  // VK_0 = 0x30
    }
    // Common symbols
    switch (c) {
        case ' ': return 0x20;  // Space
        case '-': return 0xBD;  // Minus
        case '=': return 0xBB;  // Equal
        case '[': return 0xDB;  // Left bracket
        case ']': return 0xDD;  // Right bracket
        case '\\': return 0xDC; // Backslash
        case ';': return 0xBA;  // Semicolon
        case '\'': return 0xDE; // Apostrophe
        case ',': return 0xBC;  // Comma
        case '.': return 0xBE;  // Period
        case '/': return 0xBF;  // Slash
        case '`': return 0xC0;  // Grave accent
        default: return 0;       // Unknown
    }
}

int hexToInt(const std::string& hex) {
    // Hexadecimal format: 0x41 or 0X41
    if (hex.substr(0, 2) == "0x" || hex.substr(0, 2) == "0X") {
        return std::stoi(hex, nullptr, 16);
    }
    
    // Decimal format: 65
    if (hex.length() >= 1 && std::isdigit(hex[0])) {
        return std::stoi(hex);
    }
    
    // Single character format: A, B, [, etc.
    if (hex.length() == 1) {
        int vk = charToVK(hex[0]);
        if (vk != 0) {
            return vk;
        }
    }
    
    // Attempt direct decimal conversion
    try {
        return std::stoi(hex);
    } catch (...) {
        std::cerr << "ERROR: Unable to convert '" << hex << "' to VK code" << std::endl;
        return 0;
    }
}

std::string intToHex(int value) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << value;
    return ss.str();
}

std::string vkToChar(int vk) {
    // Convert a VK code to readable character for display
    if (vk >= 0x41 && vk <= 0x5A) {
        // Letters A-Z
        char c = 'A' + (vk - 0x41);
        return std::string(1, c);
    }
    if (vk >= 0x30 && vk <= 0x39) {
        // Digits 0-9
        char c = '0' + (vk - 0x30);
        return std::string(1, c);
    }
    // Common symbols
    switch (vk) {
        case 0x20: return "SPACE";
        case 0xBA: return "; or $";
        case 0xBB: return "= or +";
        case 0xBC: return ",";
        case 0xBD: return "- or _";
        case 0xBE: return ". or >";
        case 0xBF: return "/ or ?";
        case 0xC0: return "` or ~";
        case 0xDB: return "[ or {";
        case 0xDC: return "\\ or |";
        case 0xDD: return "] or }";
        case 0xDE: return "' or \"";
        default: return intToHex(vk);
    }
}

// ========================================
// SAVE FILE MANAGEMENT
// ========================================

std::string escapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c == '\n') result += "\\n";
        else if (c == '\r') result += "\\r";
        else if (c == '\\') result += "\\\\";
        else if (c == '|') result += "\\p";
        else result += c;
    }
    return result;
}

std::string unescapeString(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            if (str[i + 1] == 'n') { result += '\n'; i++; }
            else if (str[i + 1] == 'r') { result += '\r'; i++; }
            else if (str[i + 1] == '\\') { result += '\\'; i++; }
            else if (str[i + 1] == 'p') { result += '|'; i++; }
            else result += str[i];
        } else {
            result += str[i];
        }
    }
    return result;
}

void loadKeyConfiguration() {
    std::ifstream file(SAVE_FILE);
    if (!file.is_open()) {
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Load key configuration
        if (line.substr(0, 10) == "KEY_SAVE1=") {
            std::string value = line.substr(10);
            KEY_SAVE1 = hexToInt(value);
        }
        else if (line.substr(0, 10) == "KEY_SAVE2=") {
            std::string value = line.substr(10);
            KEY_SAVE2 = hexToInt(value);
        }
        else if (line.substr(0, 9) == "KEY_LOAD=") {
            std::string value = line.substr(9);
            KEY_LOAD = hexToInt(value);
        }
        else if (line.substr(0, 11) == "SLOT_CHARS=") {
            std::string value = line.substr(11);
            // Parse comma-separated characters
            std::stringstream ss(value);
            std::string token;
            int idx = 0;
            while (std::getline(ss, token, ',') && idx < 10) {
                SLOT_CHARS[idx] = token;
                idx++;
            }
        }
        else if (line.substr(0, 4) == "SLOT") {
            // We've reached the slots, stop
            break;
        }
    }
    
    file.close();
}

void initializeSaveFile() {
    std::ifstream testFile(SAVE_FILE);
    if (testFile.is_open()) {
        testFile.close();
        // Load configuration from existing file
        loadKeyConfiguration();
        return;
    }
    
    // Create file with default configuration and 10 empty slots
    std::ofstream file(SAVE_FILE, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "ERROR: Unable to create save file" << std::endl;
        MessageBoxA(NULL, "Unable to create save file!", "Error", MB_ICONERROR);
        return;
    }
    
    // Write key configuration
    file << "# ========================================" << std::endl;
    file << "# KEY CONFIGURATION" << std::endl;
    file << "# ========================================" << std::endl;
    file << "# Three formats are accepted:" << std::endl;
    file << "#   1. SINGLE LETTER     : KEY_SAVE1=A      (simplest!)" << std::endl;
    file << "#   2. HEXADECIMAL CODE  : KEY_SAVE1=0x41   (technical format)" << std::endl;
    file << "#   3. DECIMAL CODE      : KEY_SAVE1=65     (alternative format)" << std::endl;
    file << "#" << std::endl;
    file << "# Examples of simple letters:" << std::endl;
    file << "#   Letters : A, B, C, D ... Z" << std::endl;
    file << "#   Digits  : 0, 1, 2 ... 9 (warning: not for SAVE/LOAD!)" << std::endl;
    file << "#   Symbols : [ ] \\ ; ' , . / - = `" << std::endl;
    file << "#" << std::endl;
    file << "# Concrete examples:" << std::endl;
    file << "#   To use the A key : KEY_SAVE1=A" << std::endl;
    file << "#   To use the [ key : KEY_SAVE1=[" << std::endl;
    file << "#   To use the ; key : KEY_SAVE1=;" << std::endl;
    file << "#" << std::endl;
    file << "# Common VK codes (if hexadecimal format needed):" << std::endl;
    file << "#   Letters A-Z : 0x41 to 0x5A" << std::endl;
    file << "#   Digits 0-9  : 0x30 to 0x39" << std::endl;
    file << "#   $ or ;      : 0xBA" << std::endl;
    file << "#   ^ or '      : 0xDE" << std::endl;
    file << "#   L or ]      : 0xDD" << std::endl;
    file << "#   [ or {      : 0xDB" << std::endl;
    file << "#" << std::endl;
    file << "# Save key 1 (default: $ = 0xBA)" << std::endl;
    file << "KEY_SAVE1=" << intToHex(KEY_SAVE1) << std::endl;
    file << "#" << std::endl;
    file << "# Save key 2 (default: L = 0xDD)" << std::endl;
    file << "KEY_SAVE2=" << intToHex(KEY_SAVE2) << std::endl;
    file << "#" << std::endl;
    file << "# Load key (default: ^ = 0xDE)" << std::endl;
    file << "KEY_LOAD=" << intToHex(KEY_LOAD) << std::endl;
    file << "#" << std::endl;
    file << "# Characters displayed for slots 1-10 (keys with Shift)" << std::endl;
    file << "# You can change them to match your keyboard" << std::endl;
    file << "SLOT_CHARS=&,é,\",',\\(,-,è,_,ç,à" << std::endl;
    file << "#" << std::endl;
    file << "# ========================================" << std::endl;
    file << "# CLIPBOARD SLOTS" << std::endl;
    file << "# ========================================" << std::endl;
    file << "#" << std::endl;
    
    // Create 10 empty slots
    for (int i = 1; i <= 10; i++) {
        file << "SLOT" << i << "|" << std::endl;
    }
    
    file.close();
}

std::string readSlotFromFile(const std::string& slotNum) {
    std::ifstream file(SAVE_FILE);
    if (!file.is_open()) {
        return "";
    }
    
    std::string line;
    std::string targetPrefix = "SLOT" + slotNum + "|";
    
    while (std::getline(file, line)) {
        // Ignore configuration lines and comments
        if (line.empty() || line[0] == '#' || line.substr(0, 4) == "KEY_" || line.substr(0, 10) == "SLOT_CHARS") {
            continue;
        }
        
        if (line.substr(0, targetPrefix.length()) == targetPrefix) {
            std::string content = line.substr(targetPrefix.length());
            file.close();
            return unescapeString(content);
        }
    }
    
    file.close();
    return "";
}

bool writeSlotToFile(const std::string& slotNum, const std::string& content) {
    // Read entire file
    std::ifstream fileIn(SAVE_FILE);
    if (!fileIn.is_open()) {
        return false;
    }
    
    std::vector<std::string> configLines;
    std::map<std::string, std::string> slots;
    std::string line;
    
    while (std::getline(fileIn, line)) {
        // Save configuration lines
        if (line.empty() || line[0] == '#' || line.substr(0, 4) == "KEY_" || line.substr(0, 10) == "SLOT_CHARS") {
            configLines.push_back(line);
        }
        else {
            size_t pipePos = line.find('|');
            if (pipePos != std::string::npos && line.substr(0, 4) == "SLOT") {
                std::string key = line.substr(4, pipePos - 4);
                std::string value = line.substr(pipePos + 1);
                slots[key] = value;
            }
        }
    }
    fileIn.close();
    
    // Update or add slot
    slots[slotNum] = escapeString(content);
    
    // Write entire file
    std::ofstream fileOut(SAVE_FILE, std::ios::out | std::ios::trunc);
    if (!fileOut.is_open()) {
        return false;
    }
    
    // Write configuration lines
    for (const auto& configLine : configLines) {
        fileOut << configLine << std::endl;
    }
    
    // First write slots 1-10 in order
    for (int i = 1; i <= 10; i++) {
        std::string key = std::to_string(i);
        if (slots.find(key) != slots.end()) {
            fileOut << "SLOT" << key << "|" << slots[key] << std::endl;
        } else {
            fileOut << "SLOT" << key << "|" << std::endl;
        }
    }
    
    // Then write all other slots (sorted numerically if possible)
    std::vector<std::pair<std::string, std::string>> otherSlots;
    for (const auto& slot : slots) {
        try {
            int num = std::stoi(slot.first);
            if (num < 1 || num > 10) {
                otherSlots.push_back(slot);
            }
        } catch (...) {
            otherSlots.push_back(slot);
        }
    }
    
    // Sort other slots
    std::sort(otherSlots.begin(), otherSlots.end(), [](const auto& a, const auto& b) {
        try {
            int numA = std::stoi(a.first);
            int numB = std::stoi(b.first);
            return numA < numB;
        } catch (...) {
            return a.first < b.first;
        }
    });
    
    for (const auto& slot : otherSlots) {
        fileOut << "SLOT" << slot.first << "|" << slot.second << std::endl;
    }
    
    fileOut.close();
    return true;
}

void clearNonPrimarySlots() {
    std::ifstream fileIn(SAVE_FILE);
    if (!fileIn.is_open()) {
        return;
    }
    
    std::vector<std::string> configLines;
    std::map<int, std::string> primarySlots;
    std::string line;
    
    while (std::getline(fileIn, line)) {
        // Save configuration lines
        if (line.empty() || line[0] == '#' || line.substr(0, 4) == "KEY_" || line.substr(0, 10) == "SLOT_CHARS") {
            configLines.push_back(line);
        }
        else {
            size_t pipePos = line.find('|');
            if (pipePos != std::string::npos && line.substr(0, 4) == "SLOT") {
                std::string key = line.substr(4, pipePos - 4);
                std::string value = line.substr(pipePos + 1);
                
                try {
                    int num = std::stoi(key);
                    if (num >= 1 && num <= 10) {
                        primarySlots[num] = value;
                    }
                } catch (...) {
                    // Ignore non-numeric slots
                }
            }
        }
    }
    fileIn.close();
    
    // Rewrite only config lines and primary slots (1-10)
    std::ofstream fileOut(SAVE_FILE, std::ios::out | std::ios::trunc);
    if (!fileOut.is_open()) {
        return;
    }
    
    // Write configuration lines
    for (const auto& configLine : configLines) {
        fileOut << configLine << std::endl;
    }
    
    // Write slots 1-10
    for (int i = 1; i <= 10; i++) {
        if (primarySlots.find(i) != primarySlots.end()) {
            fileOut << "SLOT" << i << "|" << primarySlots[i] << std::endl;
        } else {
            fileOut << "SLOT" << i << "|" << std::endl;
        }
    }
    
    fileOut.close();
}

bool clearSpecificSlot(const std::string& slotNum) {
    // Read entire file
    std::ifstream fileIn(SAVE_FILE);
    if (!fileIn.is_open()) {
        return false;
    }
    
    std::vector<std::string> configLines;
    std::map<std::string, std::string> slots;
    std::string line;
    
    while (std::getline(fileIn, line)) {
        // Save configuration lines
        if (line.empty() || line[0] == '#' || line.substr(0, 4) == "KEY_" || line.substr(0, 10) == "SLOT_CHARS") {
            configLines.push_back(line);
        }
        else {
            size_t pipePos = line.find('|');
            if (pipePos != std::string::npos && line.substr(0, 4) == "SLOT") {
                std::string key = line.substr(4, pipePos - 4);
                std::string value = line.substr(pipePos + 1);
                slots[key] = value;
            }
        }
    }
    fileIn.close();
    
    // Check if slot exists
    if (slots.find(slotNum) == slots.end()) {
        return false;
    }
    
    // Determine if it's a primary slot (1-10) or not
    bool isPrimarySlot = false;
    try {
        int num = std::stoi(slotNum);
        if (num >= 1 && num <= 10) {
            isPrimarySlot = true;
        }
    } catch (...) {
        isPrimarySlot = false;
    }
    
    if (isPrimarySlot) {
        // Primary slot: empty content but keep slot
        slots[slotNum] = "";
    } else {
        // Non-primary slot: delete completely
        slots.erase(slotNum);
    }
    
    // Write entire file
    std::ofstream fileOut(SAVE_FILE, std::ios::out | std::ios::trunc);
    if (!fileOut.is_open()) {
        return false;
    }
    
    // Write configuration lines
    for (const auto& configLine : configLines) {
        fileOut << configLine << std::endl;
    }
    
    // First write slots 1-10 in order
    for (int i = 1; i <= 10; i++) {
        std::string key = std::to_string(i);
        if (slots.find(key) != slots.end()) {
            fileOut << "SLOT" << key << "|" << slots[key] << std::endl;
        } else {
            fileOut << "SLOT" << key << "|" << std::endl;
        }
    }
    
    // Then write all other slots (sorted numerically if possible)
    std::vector<std::pair<std::string, std::string>> otherSlots;
    for (const auto& slot : slots) {
        try {
            int num = std::stoi(slot.first);
            if (num < 1 || num > 10) {
                otherSlots.push_back(slot);
            }
        } catch (...) {
            otherSlots.push_back(slot);
        }
    }
    
    // Sort other slots
    std::sort(otherSlots.begin(), otherSlots.end(), [](const auto& a, const auto& b) {
        try {
            int numA = std::stoi(a.first);
            int numB = std::stoi(b.first);
            return numA < numB;
        } catch (...) {
            return a.first < b.first;
        }
    });
    
    for (const auto& slot : otherSlots) {
        fileOut << "SLOT" << slot.first << "|" << slot.second << std::endl;
    }
    
    fileOut.close();
    return true;
}

void displayAllSlots() {
    std::ifstream file(SAVE_FILE);
    if (!file.is_open()) {
        std::cout << "ERROR: Unable to open file" << std::endl;
        return;
    }
    
    std::cout << "=========================================================" << std::endl;
    std::cout << "                  ACTIVE SLOTS                           " << std::endl;
    std::cout << "=========================================================" << std::endl;
    
    std::map<std::string, std::string> allSlots;
    std::string line;
    
    while (std::getline(file, line)) {
        // Ignore configuration lines and comments
        if (line.empty() || line[0] == '#' || line.substr(0, 4) == "KEY_" || line.substr(0, 10) == "SLOT_CHARS") {
            continue;
        }
        
        size_t pipePos = line.find('|');
        if (pipePos != std::string::npos && line.substr(0, 4) == "SLOT") {
            std::string key = line.substr(4, pipePos - 4);
            std::string value = line.substr(pipePos + 1);
            if (!value.empty()) {
                allSlots[key] = unescapeString(value);
            }
        }
    }
    file.close();
    
    // Display primary slots (1-10)
    for (int i = 1; i <= 10; i++) {
        std::string key = std::to_string(i);
        std::cout << "  Slot " << i << " [key " << (i == 10 ? "0/" : std::to_string(i) + "/") << SLOT_CHARS[i-1] << "] : ";
        
        if (allSlots.find(key) != allSlots.end()) {
            std::string content = allSlots[key];
            std::string display = content;
            for (size_t j = 0; j < display.length(); j++) {
                if (display[j] == '\n' || display[j] == '\r' || display[j] == '\t') {
                    display[j] = ' ';
                }
            }
            
            if (display.length() > 50) {
                std::cout << "\"" << display.substr(0, 47) << "...\"" << std::endl;
            } else {
                std::cout << "\"" << display << "\"" << std::endl;
            }
        } else {
            std::cout << "[EMPTY]" << std::endl;
        }
    }
    
    // Display other slots
    std::vector<std::pair<std::string, std::string>> otherSlots;
    for (const auto& slot : allSlots) {
        try {
            int num = std::stoi(slot.first);
            if (num < 1 || num > 10) {
                otherSlots.push_back(slot);
            }
        } catch (...) {
            otherSlots.push_back(slot);
        }
    }
    
    if (!otherSlots.empty()) {
        std::cout << "\n--- ADDITIONAL SLOTS ---" << std::endl;
        
        std::sort(otherSlots.begin(), otherSlots.end(), [](const auto& a, const auto& b) {
            try {
                int numA = std::stoi(a.first);
                int numB = std::stoi(b.first);
                return numA < numB;
            } catch (...) {
                return a.first < b.first;
            }
        });
        
        for (const auto& slot : otherSlots) {
            std::cout << "  Slot [" << slot.first << "] : ";
            
            std::string display = slot.second;
            for (size_t j = 0; j < display.length(); j++) {
                if (display[j] == '\n' || display[j] == '\r' || display[j] == '\t') {
                    display[j] = ' ';
                }
            }
            
            if (display.length() > 50) {
                std::cout << "\"" << display.substr(0, 47) << "...\"" << std::endl;
            } else {
                std::cout << "\"" << display << "\"" << std::endl;
            }
        }
    }
    
    std::cout << "=========================================================" << std::endl;
}

void refreshDisplay() {
    if (!g_consoleVisible) return;
    
    system("cls");
    
    // Display last 4 action history
    displayHistory();
    
    // Display all slots
    displayAllSlots();
}

// ========================================
// CLIPBOARD MANAGEMENT
// ========================================

std::string getClipboard() {
    if (!OpenClipboard(nullptr)) {
        return "";
    }
    
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == nullptr) {
        CloseClipboard();
        return "";
    }
    
    wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
    if (pszText == nullptr) {
        CloseClipboard();
        return "";
    }
    
    int size = WideCharToMultiByte(CP_UTF8, 0, pszText, -1, nullptr, 0, nullptr, nullptr);
    std::string text(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, pszText, -1, &text[0], size, nullptr, nullptr);
    
    GlobalUnlock(hData);
    CloseClipboard();
    
    if (!text.empty() && text.back() == '\0') {
        text.pop_back();
    }
    
    return text;
}

bool setClipboard(const std::string& text) {
    if (!OpenClipboard(nullptr)) {
        return false;
    }
    
    EmptyClipboard();
    
    int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size * sizeof(wchar_t));
    if (hMem == nullptr) {
        CloseClipboard();
        return false;
    }
    
    wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
    if (pMem == nullptr) {
        GlobalFree(hMem);
        CloseClipboard();
        return false;
    }
    
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, pMem, size);
    GlobalUnlock(hMem);
    
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
    
    return true;
}

// ========================================
// SYSTEM TRAY ICON
// ========================================

void AddTrayIcon(HWND hwnd) {
    memset(&nid, 0, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    strcpy_s(nid.szTip, sizeof(nid.szTip), "Clipboard Manager");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowTrayMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU hMenu = CreatePopupMenu();
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_TOGGLE_CONSOLE, g_consoleVisible ? "Hide console" : "Show console");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_ABOUT, "About");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");
    
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

// ========================================
// KEYBOARD HOOK
// ========================================

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) {
        return CallNextHookEx(g_hook, nCode, wParam, lParam);
    }
    
    KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
    int vkCode = kbStruct->vkCode;
    bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
    bool isKeyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
    
    // ESC to exit
    if (isKeyDown && vkCode == VK_ESCAPE) {
        PostMessage(g_hwnd, WM_CLOSE, 0, 0);
        return 1;
    }
    
    // ========== SPECIAL KEY HANDLING ==========
    
    if (vkCode == KEY_SAVE1 || vkCode == KEY_SAVE2 || vkCode == KEY_LOAD) {
        if (isKeyDown) {
            if (!keyPressed[vkCode]) {
                keyPressed[vkCode] = true;
                actionExecuted[vkCode] = false;
            }
            
            // LOAD + SAVE1 or LOAD + SAVE2 = CLEAR ALL SLOTS (except primary)
            if ((vkCode == KEY_SAVE1 || vkCode == KEY_SAVE2) && keyPressed[KEY_LOAD] && !isAccumulatingSlot) {
                clearNonPrimarySlots();
                addToHistory("OK CLEAR --> All additional slots deleted");
                refreshDisplay();
                actionExecuted[KEY_SAVE1] = true;
                actionExecuted[KEY_SAVE2] = true;
                actionExecuted[KEY_LOAD] = true;
                return 1;
            }
            
            // SAVE1 + LOAD or SAVE2 + LOAD = TOGGLE CONSOLE
            if (vkCode == KEY_LOAD && (keyPressed[KEY_SAVE1] || keyPressed[KEY_SAVE2]) && !isAccumulatingSlot) {
                toggleConsole();
                if (g_consoleVisible) {
                    refreshDisplay();
                }
                actionExecuted[KEY_SAVE1] = true;
                actionExecuted[KEY_SAVE2] = true;
                actionExecuted[KEY_LOAD] = true;
                return 1;
            }
            
            return 1;
        }
        else if (isKeyUp) {
            // Release SAVE1 or SAVE2 = SAVE
            if ((vkCode == KEY_SAVE1 || vkCode == KEY_SAVE2) && isAccumulatingSlot && !currentSlotNumber.empty() && !isClearMode) {
                // Convert slot according to rules
                std::string finalSlot = currentSlotNumber;
                if (currentSlotNumber == "0") {
                    finalSlot = "10";
                }
                
                // Save
                std::string clipContent = getClipboard();
                bool success = writeSlotToFile(finalSlot, clipContent);
                
                if (success) {
                    std::string preview = clipContent;
                    if (preview.length() > 40) {
                        preview = preview.substr(0, 37) + "...";
                    }
                    for (size_t j = 0; j < preview.length(); j++) {
                        if (preview[j] == '\n' || preview[j] == '\r' || preview[j] == '\t') {
                            preview[j] = ' ';
                        }
                    }
                    
                    std::string action = "OK SAVE --> Slot [" + finalSlot + "] : \"" + preview + "\"";
                    addToHistory(action);
                    refreshDisplay();
                }
                
                // Reset accumulation
                isAccumulatingSlot = false;
                currentSlotNumber = "";
                actionExecuted[KEY_SAVE1] = true;
                actionExecuted[KEY_SAVE2] = true;
            }
            
            // Release LOAD = LOAD or CLEAR
            if (vkCode == KEY_LOAD && isAccumulatingSlot && !currentSlotNumber.empty()) {
                // Convert slot according to rules
                std::string finalSlot = currentSlotNumber;
                if (currentSlotNumber == "0") {
                    finalSlot = "10";
                }
                
                if (isClearMode) {
                    // CLEAR MODE: Empty or delete slot
                    bool success = clearSpecificSlot(finalSlot);
                    
                    if (success) {
                        // Check if it's a primary slot
                        bool isPrimary = false;
                        try {
                            int num = std::stoi(finalSlot);
                            if (num >= 1 && num <= 10) {
                                isPrimary = true;
                            }
                        } catch (...) {
                            isPrimary = false;
                        }
                        
                        if (isPrimary) {
                            std::string action = "OK CLEAR --> Slot [" + finalSlot + "] emptied";
                            addToHistory(action);
                        } else {
                            std::string action = "OK DELETE --> Slot [" + finalSlot + "] deleted";
                            addToHistory(action);
                        }
                        refreshDisplay();
                    } else {
                        std::string action = "XX ERROR --> Slot [" + finalSlot + "] not found";
                        addToHistory(action);
                        refreshDisplay();
                    }
                } else {
                    // NORMAL MODE: Load
                    std::string content = readSlotFromFile(finalSlot);
                    
                    if (!content.empty()) {
                        bool success = setClipboard(content);
                        if (success) {
                            std::string preview = content;
                            if (preview.length() > 40) {
                                preview = preview.substr(0, 37) + "...";
                            }
                            for (size_t j = 0; j < preview.length(); j++) {
                                if (preview[j] == '\n' || preview[j] == '\r' || preview[j] == '\t') {
                                    preview[j] = ' ';
                                }
                            }
                            
                            std::string action = "OK LOAD <-- Slot [" + finalSlot + "] : \"" + preview + "\"";
                            addToHistory(action);
                            refreshDisplay();
                        }
                    } else {
                        std::string action = "XX ERROR --> Slot [" + finalSlot + "] is EMPTY";
                        addToHistory(action);
                        refreshDisplay();
                    }
                }
                
                // Reset accumulation and CLEAR mode
                isAccumulatingSlot = false;
                currentSlotNumber = "";
                isClearMode = false;
                actionExecuted[KEY_LOAD] = true;
            }
            
            keyPressed[vkCode] = false;
            
            // If no action was performed, simulate the character
            if (!actionExecuted[vkCode]) {
                INPUT input[2] = {};
                input[0].type = INPUT_KEYBOARD;
                input[0].ki.wVk = vkCode;
                input[1].type = INPUT_KEYBOARD;
                input[1].ki.wVk = vkCode;
                input[1].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(2, input, sizeof(INPUT));
            }
            
            actionExecuted[vkCode] = false;
            return 1;
        }
    }
    
    // ========== C KEY HANDLING (CLEAR MODE) ==========
    
    if (vkCode == KEY_CLEAR) {
        if (isKeyDown && keyPressed[KEY_LOAD] && !keyPressed[vkCode]) {
            // LOAD + C = Activate CLEAR mode
            keyPressed[KEY_CLEAR] = true;
            isClearMode = true;
            return 1;
        }
        else if (isKeyUp) {
            keyPressed[KEY_CLEAR] = false;
            return 1;
        }
    }
    
    // ========== SLOT KEY HANDLING ==========
    
    bool isSlotKey = false;
    std::string slotDigit = "";
    for (int i = 0; i < 10; i++) {
        if (vkCode == slotKeys[i]) {
            isSlotKey = true;
            if (i == 9) {
                slotDigit = "0";
            } else {
                slotDigit = std::to_string(i + 1);
            }
            break;
        }
    }
    
    if (isSlotKey && isKeyDown) {
        // Check if SAVE1 or SAVE2 is held (ACCUMULATION for SAVE)
        if (keyPressed[KEY_SAVE1] || keyPressed[KEY_SAVE2]) {
            if (!isAccumulatingSlot) {
                isAccumulatingSlot = true;
                currentSlotNumber = slotDigit;
            } else {
                currentSlotNumber += slotDigit;
            }
            
            actionExecuted[KEY_SAVE1] = true;
            actionExecuted[KEY_SAVE2] = true;
            return 1;
        }
        // Check if LOAD is held (ACCUMULATION for LOAD)
        else if (keyPressed[KEY_LOAD]) {
            if (!isAccumulatingSlot) {
                isAccumulatingSlot = true;
                currentSlotNumber = slotDigit;
            } else {
                currentSlotNumber += slotDigit;
            }
            
            actionExecuted[KEY_LOAD] = true;
            return 1;
        }
    }
    
    // Block slot keys if a special key is active
    if (isSlotKey && (keyPressed[KEY_SAVE1] || keyPressed[KEY_SAVE2] || keyPressed[KEY_LOAD])) {
        return 1;
    }
    
    // Block C key if LOAD is held
    if (vkCode == KEY_CLEAR && keyPressed[KEY_LOAD]) {
        return 1;
    }
    
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

// ========================================
// WINDOW PROCEDURE
// ========================================

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu(hwnd);
            }
            break;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            else if (LOWORD(wParam) == ID_TRAY_TOGGLE_CONSOLE) {
                toggleConsole();
                if (g_consoleVisible) {
                    refreshDisplay();
                }
            }
            else if (LOWORD(wParam) == ID_TRAY_ABOUT) {
                std::string aboutMsg = 
                    "Multi-Slot Clipboard Manager\n"
                    "Configurable Version - Customizable keys\n\n"
                    "SAVE:\n"
                    "Hold SAVE key + slot keys\n"
                    "-> Accumulates digits\n"
                    "-> Saves on release\n\n"
                    "LOAD:\n"
                    "Hold LOAD key + slot keys\n"
                    "-> Accumulates digits\n"
                    "-> Loads on release\n\n"
                    "CLEAR A SLOT:\n"
                    "Hold LOAD + C + slot keys\n"
                    "-> Slots 1-10: empties content\n"
                    "-> Other slots: deletes completely\n\n"
                    "TOGGLE CONSOLE:\n"
                    "SAVE + LOAD\n\n"
                    "CLEAR ADDITIONAL SLOTS:\n"
                    "LOAD + SAVE\n\n"
                    "CONFIGURATION:\n"
                    "Edit clipboard_slots.dat to change keys\n"
                    "KEY_SAVE1, KEY_SAVE2, KEY_LOAD, SLOT_CHARS\n\n"
                    "EXIT: ESC key";
                MessageBoxA(hwnd, aboutMsg.c_str(), "About", MB_ICONINFORMATION);
            }
            break;
            
        case WM_CLOSE:
        case WM_DESTROY:
            if (g_hook) {
                UnhookWindowsHookEx(g_hook);
                g_hook = NULL;
            }
            RemoveTrayIcon();
            g_running = false;
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// ========================================
// MAIN FUNCTION
// ========================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Create a console VISIBLE at startup
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    
    // Get console handle
    g_console = GetConsoleWindow();
    
    if (g_console) {
        ShowWindow(g_console, SW_SHOW);
        SetForegroundWindow(g_console);
    }
    
    std::cout << "=========================================================" << std::endl;
    std::cout << "   CLIPBOARD MANAGER - CONFIGURABLE VERSION   " << std::endl;
    std::cout << "=========================================================" << std::endl;
    
    // INITIALIZE SAVE FILE
    std::cout << "\n[INIT] Initializing save file..." << std::endl;
    initializeSaveFile();
    std::cout << "OK Save file ready: " << SAVE_FILE << std::endl;
    
    std::cout << "\n[CONFIG] Key configuration:" << std::endl;
    std::cout << "  - SAVE1 (save): " << vkToChar(KEY_SAVE1) << " [" << intToHex(KEY_SAVE1) << "]" << std::endl;
    std::cout << "  - SAVE2 (save): " << vkToChar(KEY_SAVE2) << " [" << intToHex(KEY_SAVE2) << "]" << std::endl;
    std::cout << "  - LOAD (load):  " << vkToChar(KEY_LOAD) << " [" << intToHex(KEY_LOAD) << "]" << std::endl;
    std::cout << "  - Slot characters: ";
    for (int i = 0; i < 10; i++) {
        std::cout << SLOT_CHARS[i];
        if (i < 9) std::cout << ",";
    }
    std::cout << std::endl;
    
    std::cout << "\n=========================================================" << std::endl;
    std::cout << "                     USAGE                         " << std::endl;
    std::cout << "=========================================================" << std::endl;
    std::cout << "\n1. SAVE (infinite slots):" << std::endl;
    std::cout << "   Hold SAVE key + number keys" << std::endl;
    std::cout << "   Ex: SAVE + 1 + 2 + 3 then release = slot 123" << std::endl;
    std::cout << "\n2. LOAD (infinite slots):" << std::endl;
    std::cout << "   Hold LOAD key + number keys" << std::endl;
    std::cout << "   Ex: LOAD + 4 + 5 then release = slot 45" << std::endl;
    std::cout << "\n3. CLEAR A SLOT:" << std::endl;
    std::cout << "   Hold LOAD + C + number keys" << std::endl;
    std::cout << "   Ex: LOAD + C + 1 + 1 then release = clear slot 11" << std::endl;
    std::cout << "\n4. TOGGLE CONSOLE:" << std::endl;
    std::cout << "   SAVE + LOAD" << std::endl;
    std::cout << "\n5. CLEAR ADDITIONAL SLOTS:" << std::endl;
    std::cout << "   LOAD + SAVE" << std::endl;
    std::cout << "\n6. CONFIGURATION:" << std::endl;
    std::cout << "   Edit " << SAVE_FILE << " to customize keys" << std::endl;
    std::cout << "\n7. EXIT:" << std::endl;
    std::cout << "   ESC key" << std::endl;
    std::cout << "\n=========================================================" << std::endl;
    
    // Create window class
    const char CLASS_NAME[] = "ClipboardManager";
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassA(&wc);
    
    // Create invisible window
    g_hwnd = CreateWindowExA(0, CLASS_NAME, "Clipboard Manager", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, NULL, NULL, hInstance, NULL);
    
    if (!g_hwnd) {
        MessageBoxA(NULL, "Window creation error", "Error", MB_ICONERROR);
        return 1;
    }
    
    // Add system tray icon
    AddTrayIcon(g_hwnd);
    
    // Install keyboard hook
    g_hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);
    if (!g_hook) {
        MessageBoxA(NULL, "Keyboard hook installation error", "Error", MB_ICONERROR);
        return 1;
    }
    
    std::cout << "\nOK Program active!" << std::endl;
    std::cout << "OK Icon in system tray" << std::endl;
    std::cout << "OK Configurable keys in save file" << std::endl;
    std::cout << "\n[READY] Awaiting commands...\n" << std::endl;
    
    // Initial display
    Sleep(500);
    refreshDisplay();
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}