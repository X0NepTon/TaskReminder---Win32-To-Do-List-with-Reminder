# TaskReminder - Win32 To-Do List with Reminder

## Overview

TaskReminder is a desktop application developed in **C++** using the **Win32 API**. It provides a simple and user-friendly To-Do List with a built-in reminder system. Users can create, edit, delete, save, and load tasks while receiving notifications at the scheduled time.

## Features

* Add new tasks
* Edit existing tasks
* Delete tasks
* Display all tasks in a ListBox
* Set reminder time using **12-hour format (AM/PM)**
* Automatic reminder notifications
* Save tasks to a text file
* Load tasks from a text file
* Prevent duplicate reminders on the same day
* Simple graphical user interface built entirely with the Win32 API

## Technologies Used

* C++
* Win32 API
* Windows Common Controls (`commctrl`)
* Windows Common Dialog (`commdlg`)
* Standard Template Library (STL)
* File Handling (`fstream`)
* String Streams (`sstream`)

## Application Interface

### Main Window

* Task List (ListBox)
* Add Task button
* Edit Task button
* Delete Task button
* Save button
* Load button

### Task Dialog

* Task Title
* Task Description
* Reminder Hour
* Reminder Minute
* AM/PM Selection

## Task Structure

Each task contains the following information:

* Title
* Description
* Hour
* Minute
* Notification Status
* Last Notification Day

## Reminder System

The application checks the current system time every second using the Windows Timer API.

When the current time matches a task's reminder time:

* A reminder message is displayed.
* The reminder is shown only once per day.
* The reminder automatically becomes available again on the next day.

## File Format

Tasks are stored in a plain text file using the following format:

```text
Title|Description|Hour|Minute|LastNotifiedDay
```

Example:

```text
Study C++|Finish Win32 Project|2|30|15
```

## Requirements

* Windows 10 or Windows 11
* Visual Studio 2019 or later
* C++ compiler with Win32 API support

## Build Instructions

1. Open the project in Visual Studio.
2. Create or open a **Windows Desktop Application** project.
3. Add the source file to the project.
4. Build and run the application.

## Future Improvements

* Add a Date Picker to support scheduled dates.
* Support recurring reminders (daily, weekly, monthly).
* Search and filter tasks.
* Sort tasks by reminder time.
* Store tasks in SQLite instead of text files.
* Add System Tray support.
* Use Windows Toast Notifications instead of MessageBox.
* Improve the user interface with modern Windows controls.


## License

This project is open-source and available for educational purposes.

## Author

Developed using **C++** and the **Win32 API** as a desktop application project.
