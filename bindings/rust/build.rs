fn main() {
    // Link to the robots library
    println!("cargo:rustc-link-search=native=../../build");
    println!("cargo:rustc-link-search=native=../../cmake-build");
    println!("cargo:rustc-link-lib=dylib=robots");

    // On macOS, also link to libc++
    #[cfg(target_os = "macos")]
    println!("cargo:rustc-link-lib=dylib=c++");

    // On Linux, link to libstdc++
    #[cfg(target_os = "linux")]
    println!("cargo:rustc-link-lib=dylib=stdc++");
}
