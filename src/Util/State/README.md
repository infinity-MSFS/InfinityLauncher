# State Management System in C++

This `State` header provides a global, thread-safe state management system using the Singleton pattern. It allows developers to register, retrieve, and manage page-specific states dynamically by type
and ID.

---

## Features

* **Thread-Safe Singleton**: Access a single global state instance throughout your program.
* **Type-Specific States**: Use `std::type_index` to group states by type.
* **Dynamic Registration**: Add new page states dynamically with unique identifiers.
* **Graceful Access**: Retrieve states using `std::optional` to handle missing states without crashing.
* **Debugging Utilities**: Easily print all registered states and their contents.

---

## How to Use

### 1. Include the Header

```c++
#include "Util/State/State.hpp"
```

---

### 2. Define Your Custom Page State

All page states must inherit from the PageState base class and implement the PrintState method for debugging.

Example:

```c++
struct ProjectPageState : public Infinity::PageState {
    std::string state_var_1;
    
    ProjectPageState(std::string var1) : state_var_1(var1) {}
    
        void PrintState() const override {
            std::cout << "Project Page State: " << state_var_1 << std::endl;
        }
};
```

---

### 3. Register a Page State

Use `State::GetInstance()` to register a state with a unique ID. Ensure you use std::make_shared when creating your state object.

Example:

```c++
auto &state = Infinity::State::GetInstance();
state.RegisterPageState("project_page", std::make_shared<ProjectPageState>("Initialized Value"));
```

---

### 4. Access a Page State

Retrieve a registered state using its type and ID. Handle missing states gracefully with `std::optional`.

Example:

```c++
auto project_page_state = state.GetPageState<ProjectPageState>("project_page");

if (project_page_state) {
    project_page_state.value()->PrintState();
} else {
    std::cout << "State for 'project_page' not found!" << std::endl;
}

```

---

### **5. Remove a Page State**

Remove a state by its type and ID. If no states remain for a type, the type entry is also removed.

Example:

```c++
state.RemovePageState<ProjectPageState>("project_page");
```

---

### **6. Debug All States**

Print all registered states grouped by type and ID.

Example:

```c++
state.PrintAllStates();
```

___

### Full Example Code

```c++
#include "Util/State/State.hpp"


struct ProjectPageState : public Infinity::PageState {
std::string state_var_1;

    ProjectPageState(std::string var1) : state_var_1(var1) {}

    void PrintState() const override {
        std::cout << "Project Page State: " << state_var_1 << std::endl;
    }

};

int main() {
// Register a state
auto &state = Infinity::State::GetInstance();
state.RegisterPageState("project_page", std::make_shared<ProjectPageState>("Initialized Value"));

    // Access and print the state
    auto project_page_state = state.GetPageState<ProjectPageState>("project_page");
    if (project_page_state) {
        project_page_state.value()->PrintState();
    } else {
        std::cout << "State for 'project_page' not found!" << std::endl;
    }

    // Debug all states
    state.PrintAllStates();

    // Remove the state
    state.RemovePageState<ProjectPageState>("project_page");
    return 0;
}
```

---

## Notes

1. **Thread Safety:** The `State` class uses `std::mutex` to ensure safe concurrent access. All public methods are thread-safe.
2. **Dynamic Type Casting:** Always use the correct type when retrieving a state. The system relies on `std::type_index` to differentiate types.
3. **Custom Debugging:** Override the `PrintState` method in your custom `PageState` subclasses to provide meaningful debugging output.