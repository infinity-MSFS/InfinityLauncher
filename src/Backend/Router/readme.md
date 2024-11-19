# Router

The Router class provides a centralized page-routing mechanism in C++. It allows for dynamic page switching by
associating page IDs with render functions, making it ideal for managing multiple pages
or views in an application.

## Features

- **Centralized Routing:** Manages page routing with a singleton pattern.
- **Dynamic Page Management:** Supports dynamic pages with render functions defined at runtime.
- **Error Handling:** Throws errors if non-existent pages are accessed, improving robustness.

## Usage

### Initialization and Configuration

To set up the router, initialize it by calling `Router::configure()` with a map of page IDs and their respective render
functions. This must be done once before any other usage.

#### Example

```c++
    #include "Router.hpp"

    using namespace Infinity::Utils;
    
    void HomePage() {/* redner Home page */}    
    void PackagePage() {/* redner Package page */}
    
    int main(){
        std::unordered_map<int,std::function<void()>> pages = {
        {0,HomePage},
        {1,PackagePage}};
        
        Router::configure(pages);
        
        // router is now configured
        
    } 

```

### Accessing the Singleton Instance

Once configured, you can access the router instance with `Router::getInstance()`. Attempting to get the instance before
configuration will throw an error.

#### Example

```c++
    auto& router = Router:getInstance();
```

### Changing Pages

To change the active page, use setPage(int pageId). It sets the page to the specified ID if it exists.

#### Example

```c++
    if(router.setPage(1)) {
        std::cout << "Navigation successful" << std::endl; // we only cover the Ok case here since the Err is handled by a runtime error
     }
```

### Rendering the Current Page

To render the currently selected page, use RenderCurrentPage(). This will invoke the render function associated with the
current page ID.

#### Example

```c++
    router.RenderCurrentPage(); // This method should be called in the render loop
```

---

## API Reference

### `static void configure(std::unordered_map<int, std::function<void()>> pages);`

Configures the router with the provided map of page IDs and render functions. This must be called once before any other
operations.

- Parameters:
    - `pages`: A map where each key is a page ID, and each value is a `std::function<void()>` representing the render
      function for that page.

### `static Router& getInstance();`

Retrieves the singleton instance of the router. Throws an error if the router has not been configured.

### `bool setPage(const int pageId);`

Sets the current page to the specified page ID if it exists. Returns `true` on success. Throws an error if the page ID
does not exist.

- Parameters:
    - `pageId`: The ID of the page to set.

### `[[nodiscard]] int getPage() const;`

Returns the current page ID.

### `void RenderCurrentPage();`

Renders the current page by calling the associated render function. Throws an error if the page ID does not exist.

---

## Error Handling

If an invalid page ID is provided to `setPag`e or `RenderCurrentPage`, the router will throw a `std::runtime_error` with
a message describing the issue.

---