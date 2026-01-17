# RobotsTxt Java Bindings

Java bindings for Google's robots.txt parser library using JNI.

## Requirements

- Java 11+
- Maven
- The `librobots` and `librobots_jni` shared libraries must be built

## Building the Native Library

First, build the main robots library and JNI bindings:

```bash
cd ../..
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_JNI=ON
make
```

## Building the Java Library

```bash
mvn compile
```

## Running Tests

```bash
# Set library path and run tests
mvn test -Djava.library.path=../../build
```

## Usage

```java
import com.google.robotstxt.RobotsMatcher;
import com.google.robotstxt.RequestRate;
import com.google.robotstxt.ContentSignal;

public class Example {
    public static void main(String[] args) {
        // Use try-with-resources for automatic cleanup
        try (RobotsMatcher matcher = new RobotsMatcher()) {
            String robotsTxt = """
                User-agent: *
                Disallow: /admin/
                Crawl-delay: 2.5
                """;

            // Check if URL is allowed
            boolean allowed = matcher.isAllowed(
                robotsTxt,
                "Googlebot",
                "https://example.com/page"
            );
            System.out.println("Access: " + (allowed ? "allowed" : "disallowed"));

            // Get crawl delay
            Double delay = matcher.getCrawlDelay();
            if (delay != null) {
                System.out.println("Crawl delay: " + delay + " seconds");
            }

            // Get request rate
            RequestRate rate = matcher.getRequestRate();
            if (rate != null) {
                System.out.println("Request rate: " + rate.getRequests() +
                                   " requests per " + rate.getSeconds() + " seconds");
                System.out.println("Delay between requests: " + rate.getDelaySeconds() + " seconds");
            }

            // Check Content-Signal (if supported)
            if (RobotsMatcher.isContentSignalSupported()) {
                System.out.println("AI training allowed: " + matcher.allowsAITrain());
                System.out.println("AI input allowed: " + matcher.allowsAIInput());
                System.out.println("Search allowed: " + matcher.allowsSearch());
            }

            // Get matching line number
            System.out.println("Matched at line: " + matcher.getMatchingLine());
        }

        // Check library version
        System.out.println("Library version: " + RobotsMatcher.getVersion());

        // Validate user-agent strings
        System.out.println("Valid user-agent: " + RobotsMatcher.isValidUserAgent("Googlebot"));
    }
}
```

## API Reference

### `RobotsMatcher`

The main class for parsing and matching robots.txt rules. Implements `AutoCloseable` for use with try-with-resources.

#### Constructor

- `RobotsMatcher()` - Creates a new matcher instance

#### Instance Methods

- `isAllowed(String robotsTxt, String userAgent, String url)` - Check if URL is allowed
- `getMatchingLine()` - Line number of the last match (0 if none)
- `everSeenSpecificAgent()` - True if a specific user-agent block was found
- `getCrawlDelay()` - Crawl delay in seconds (null if not specified)
- `getRequestRate()` - Request rate limit (null if not specified)
- `getContentSignal()` - Content signal values (null if not specified)
- `allowsAITrain()` - Whether AI training is allowed (defaults to true)
- `allowsAIInput()` - Whether AI input is allowed (defaults to true)
- `allowsSearch()` - Whether search indexing is allowed (defaults to true)
- `close()` - Release native resources

#### Static Methods

- `getVersion()` - Get library version
- `isValidUserAgent(String userAgent)` - Check if user-agent is valid
- `isContentSignalSupported()` - Whether Content-Signal is compiled in

### `RequestRate`

Request rate limit class.

- `getRequests()` - Number of requests allowed
- `getSeconds()` - Time period in seconds
- `getRequestsPerSecond()` - Computed requests per second
- `getDelaySeconds()` - Computed delay between requests

### `ContentSignal`

Content signal values (tri-state: null=unset, true=yes, false=no).

- `getAiTrain()` - AI training preference
- `getAiInput()` - AI input preference
- `getSearch()` - Search indexing preference

## CMake Integration

To build the JNI library, add to CMakeLists.txt:

```cmake
option(BUILD_JNI "Build Java JNI bindings" OFF)

if(BUILD_JNI)
    find_package(JNI REQUIRED)
    include_directories(${JNI_INCLUDE_DIRS})
    add_library(robots_jni SHARED bindings/java/src/main/native/robots_jni.cc)
    target_link_libraries(robots_jni robots)
endif()
```

## License

Apache 2.0 - See the main repository LICENSE file.
