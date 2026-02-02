# 📋 Multi-Slot Clipboard Manager

## 🎯 Description

**Multi-Slot Clipboard Manager** is a powerful Windows tool that allows you to save and load an **unlimited** number of items in your clipboard. No more repetitive copy-paste! Keep your important texts in numbered slots and access them instantly.

### ✨ Key Features

- 🔢 **Unlimited slots**: 10 quick slots (keys 0-9) + unlimited additional slots (11, 12, 13... 999, etc.)
- 🎨 **Configurable keys**: Customize keys according to your keyboard (AZERTY, QWERTY, etc.)
- 💾 **Permanent save**: Your slots are preserved even after reboot
- 🖥️ **Interactive console**: View all your slots in real-time
- 🔄 **History**: See your last 4 actions
- 🌍 **Universal**: Compatible with all keyboard types

---

## 📥 Installation

### Prerequisites
- **Windows** 7/8/10/11 (64-bit)
- **C++ Compiler**: MinGW 64-bit

### Compilation

#### With MinGW (g++)
```bash
g++ -o clipboard_manager.exe clipboard_manager.cpp -mwindows -static-libgcc -static-libstdc++
```

---

## 🎮 Detailed Features

### 1. 💾 Save Content (SAVE)

#### Principle
The **SAVE** function allows you to save the current content of your clipboard into a numbered slot.

#### How to use

**For slots 1-10 (quick access):**
1. Copy your text normally (Ctrl+C)
2. Hold the **SAVE** key (default: `$` or `£`)
3. Press the **number** of the slot (1 to 9, or 0 for slot 10)
4. Release the SAVE key
5. ✅ Content is saved!

**Example:**
```
1. Copy "Hello everyone"
2. Hold $ + press 1 + release $
3. → Slot 1 now contains "Hello everyone"
```

**For slots 11+ (infinite slots):**
1. Copy your text
2. Hold the **SAVE** key
3. Type **multiple digits** to compose the slot number
4. Release the SAVE key

**Examples:**
```
$ + 1 + 5 + release $ = Slot 15
$ + 2 + 3 + 4 + release $ = Slot 234
$ + 9 + 9 + 9 + release $ = Slot 999
```

#### Special rule: Slot 0
- Press **0** alone = **Slot 10**
- For actual slot 0: impossible (reserved)

#### Confirmation
For each save, the console displays:
```
OK SAVE --> Slot [1] : "Hello everyone"
```

---

### 2. 📤 Load Content (LOAD)

#### Principle
The **LOAD** function allows you to retrieve the content of a slot and put it in your clipboard for pasting.

#### How to use

**For slots 1-10:**
1. Hold the **LOAD** key (default: `²`)
2. Press the **number** of the slot (1 to 9, or 0 for slot 10)
3. Release the LOAD key
4. ✅ Content is in your clipboard!
5. Paste with Ctrl+V

**Example:**
```
1. Hold ² + press 1 + release ²
2. The content of slot 1 is now in the clipboard
3. Press Ctrl+V to paste it
```

**For slots 11+:**
1. Hold the **LOAD** key
2. Type **multiple digits** to compose the number
3. Release the LOAD key

**Examples:**
```
² + 1 + 5 + release ² = Load slot 15
² + 2 + 3 + 4 + release ² = Load slot 234
```

#### Confirmation
For each load, the console displays:
```
OK LOAD <-- Slot [1] : "Hello everyone"
```

#### If slot is empty
```
XX ERROR --> Slot [5] is EMPTY
```

---

### 3. 🗑️ Clear a Slot (CLEAR)

#### Principle
The **CLEAR** function allows you to empty or delete a slot.

#### Behavior
- **Slots 1-10** (primary): Content is **emptied**, but slot remains in file
- **Slots 11+** (additional): Slot is **completely deleted** from file

#### How to use

**Clear a slot:**
1. Hold the **LOAD** key (default: `²`)
2. Press **C** (to activate CLEAR mode)
3. Type the **slot number** to clear
4. Release the LOAD key

**Examples:**
```
² + C + 1 + release ² = Empty slot 1
² + C + 1 + 5 + release ² = Delete slot 15
² + C + 2 + 3 + 4 + release ² = Delete slot 234
```

#### Confirmation
**For a primary slot (1-10):**
```
OK CLEAR --> Slot [1] emptied
```

**For an additional slot (11+):**
```
OK DELETE --> Slot [15] deleted
```

---

### 4. 🧹 Clear All Additional Slots (CLEAR ALL)

#### Principle
This function deletes **all additional slots** (slots 11 and beyond) while preserving slots 1-10.

#### How to use
1. Hold the **LOAD** key (default: `²`)
2. Press the **SAVE** key (default: `$` or `£`)
3. Release both keys

**Quick shortcut:**
```
² + $ (together)
```
or
```
² + £ (together)
```

#### Result
- ✅ Slots 1-10: **Preserved** (but may be empty)
- ❌ Slots 11+: **All deleted**

#### Confirmation
```
OK CLEAR --> All additional slots deleted
```

#### Usefulness
Perfect for doing a "spring cleaning" when you've created many additional slots and want to start fresh with just your 10 main slots.

---

### 5. 👁️ Show/Hide Console (TOGGLE)

#### Principle
The console displays all your slots and your action history. You can hide it to free up screen space, or show it to view your slots.

#### How to use
1. Hold a **SAVE** key (default: `$` or `£`)
2. Press the **LOAD** key (default: `²`)
3. Release both keys

**Quick shortcut:**
```
$ + ² (together)
```
or
```
£ + ² (together)
```

#### Result
- If console was **visible** → It **hides**
- If console was **hidden** → It **reappears**

---

### 6. 📊 Interactive Console

#### Description
The console is the black window that opens at startup. It displays in real-time:
- The **history** of your last 4 actions
- **All active slots** with their content (truncated preview if too long)

#### Display structure

```
[Most recent action]
[Previous action]
[Previous action]
[Previous action]

=========================================================
                  ACTIVE SLOTS
=========================================================
  Slot 1 [key 1/&] : "Hello everyone"
  Slot 2 [key 2/é] : "My email address@example.com"
  Slot 3 [key 3/"] : [EMPTY]
  Slot 4 [key 4/'] : "Important code: ABC123"
  ...
  Slot 10 [key 0/à] : [EMPTY]

--- ADDITIONAL SLOTS ---
  Slot [15] : "Important note for later"
  Slot [234] : "Other saved content"
=========================================================
```

#### Automatic refresh
The console refreshes automatically after each action (save, load, clear, etc.).

---

### 7. 📁 Save File

#### Name and location
The file is called **`clipboard_slots.dat`** and is located in the same folder as the executable.

#### File structure

```
# ========================================
# KEY CONFIGURATION
# ========================================
# Three formats are accepted:
#   1. SINGLE LETTER     : KEY_SAVE1=A
#   2. HEXADECIMAL CODE  : KEY_SAVE1=0x41
#   3. DECIMAL CODE      : KEY_SAVE1=65
#
KEY_SAVE1=0xBA
KEY_SAVE2=0xDD
KEY_LOAD=0xDE
SLOT_CHARS=&,é,",',\(,-,è,_,ç,à
#
# ========================================
# CLIPBOARD SLOTS
# ========================================
#
SLOT1|Hello everyone
SLOT2|My email address@example.com
SLOT3|
SLOT4|Important code: ABC123
...
SLOT10|
SLOT15|Important note for later
SLOT234|Other saved content
```

#### Features
- **Plain text format**: Editable with any text editor
- **Special characters**: Automatically escaped (`\n`, `\r`, etc.)
- **Automatic save**: With each slot modification
- **Persistent**: Data survives PC reboot

#### Manual editing
You can manually edit the file if needed:
1. Close the program
2. Open `clipboard_slots.dat` with Notepad
3. Modify what you want
4. Save
5. Restart the program

---

### 8. ⚙️ Key Configuration

#### Principle
You can completely customize the keys used for saving and loading. Perfect for adapting the program to your keyboard type or your preferences.

#### The 3 accepted formats

##### 1. SIMPLE LETTER Format (Recommended) 🌟
The easiest! Simply write the letter:
```
KEY_SAVE1=A
KEY_SAVE2=B
KEY_LOAD=C
```

##### 2. HEXADECIMAL Format (For experts) 🔧
With Windows VK codes:
```
KEY_SAVE1=0x41
KEY_SAVE2=0x42
KEY_LOAD=0x43
```

##### 3. DECIMAL Format (Alternative) 🔢
With decimal numbers:
```
KEY_SAVE1=65
KEY_SAVE2=66
KEY_LOAD=67
```

#### Available keys in simple format

**Letters A-Z:**
```
A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
```

**Supported symbols:**
```
[  ]  \  ;  '  ,  .  /  -  =  `
```

#### How to configure

1. **Close the program** (ESC key or right-click → Exit)
2. **Open** `clipboard_slots.dat` with Notepad
3. **Modify** the lines at the beginning:
   ```
   KEY_SAVE1=A
   KEY_SAVE2=S
   KEY_LOAD=D
   ```
4. **Save** the file
5. **Restart** the program

#### Configuration examples

**"French AZERTY" configuration (default):**
```
KEY_SAVE1=0xBA    # $ (dollar)
KEY_SAVE2=0xDD    # £ (pound sterling)
KEY_LOAD=0xDE     # ² (squared)
```

**"Simple Letters" configuration:**
```
KEY_SAVE1=A
KEY_SAVE2=S
KEY_LOAD=D
```

**"Gamer" configuration (left hand):**
```
KEY_SAVE1=Q
KEY_SAVE2=W
KEY_LOAD=E
```

**"Brackets" configuration:**
```
KEY_SAVE1=[
KEY_SAVE2=]
KEY_LOAD=\
```

#### Startup verification
At launch, the program displays your configured keys:
```
[CONFIG] Key configuration:
  - SAVE1 (save): A [0x41]
  - SAVE2 (save): S [0x53]
  - LOAD (load):  D [0x44]
```

#### Slot character configuration
The `SLOT_CHARS` line defines the characters displayed in the console for each slot:

**French AZERTY (default):**
```
SLOT_CHARS=&,é,",',\(,-,è,_,ç,à
```

**US QWERTY:**
```
SLOT_CHARS=!,@,#,$,%,^,&,*,(,)
```

These characters correspond to keys **1-9 and 0** with **Shift** pressed.

---

---

### 10. 🎯 Action History

#### Description
The program keeps in memory your **last 4 actions** and displays them at the top of the console.

#### Display format

```
OK SAVE --> Slot [1] : "Hello everyone"
OK LOAD <-- Slot [2] : "My text"
OK CLEAR --> Slot [3] emptied
XX ERROR --> Slot [5] is EMPTY
```

#### Types of messages

**Successful save:**
```
OK SAVE --> Slot [X] : "content preview..."
```

**Successful load:**
```
OK LOAD <-- Slot [X] : "content preview..."
```

**Successful clear/delete:**
```
OK CLEAR --> Slot [X] emptied
OK DELETE --> Slot [X] deleted
```

**Successful clear all:**
```
OK CLEAR --> All additional slots deleted
```

**Error (empty slot):**
```
XX ERROR --> Slot [X] is EMPTY
```

**Error (non-existent slot):**
```
XX ERROR --> Slot [X] not found
```
---

## 🎹 Keyboard Shortcut Summary

| Action | Shortcut | Description |
|--------|-----------|-------------|
| **Save** | `SAVE + digit(s) + release SAVE` | Saves clipboard to a slot |
| **Load** | `LOAD + digit(s) + release LOAD` | Loads a slot into clipboard |
| **Clear a slot** | `LOAD + C + digit(s) + release LOAD` | Empties or deletes a slot |
| **Clear all slots 11+** | `LOAD + SAVE` | Deletes all additional slots |
| **Toggle console** | `SAVE + LOAD` | Shows/hides console |
| **Exit** | `ESC` | Closes program cleanly |

**Default keys:**
- **SAVE** = `$` or `£`
- **LOAD** = `²`
- **C** = C key (fixed)

---

---

## 💀 Possible Bug

At program launch, a message indicating missing file(s)

# Solution

Go to the website:
https://winlibs.com
Get the 64-bit version, not 32-bit

Extract to C:

Copy the bin path: C:\mingw64\bin

Do:
- Win+R
- sysdm.cpl
- Advanced system settings
- Environment variables
- Path
- New
- Paste the bin path (C:\mingw64\bin)

---

---

## 📊 Technical Limits

| Feature | Limit |
|----------------|--------|
| Number of slots | Unlimited (limited by disk memory) |
| Maximum size per slot | Unlimited (limited by RAM memory) |
| Size of clipboard_slots.dat file | Unlimited (limited by disk space) |
| Action history | Last 4 actions |
| Supported data types | Unicode text only |
| Platform | Windows only (7/8/10/11) |

---

## 📝 License and Credits

### License
This software is provided "as is", without warranty of any kind. You are free to use, modify and distribute it.

### Author
Multi-Slot Clipboard Manager
Configurable Version - 2025
Created via AI and supervised and modified by Loïs Beeckmans