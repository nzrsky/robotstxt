use robotstxt::DefaultMatcher;
use std::env;
use std::fs::File;
use std::io::{BufReader, Read};

fn load_robots_files(filename: &str) -> std::io::Result<Vec<String>> {
    let file = File::open(filename)?;
    let mut reader = BufReader::new(file);
    let mut files = Vec::new();

    loop {
        let mut len_buf = [0u8; 4];
        match reader.read_exact(&mut len_buf) {
            Ok(_) => {}
            Err(e) if e.kind() == std::io::ErrorKind::UnexpectedEof => break,
            Err(e) => return Err(e),
        }

        let length = u32::from_le_bytes(len_buf) as usize;
        let mut data = vec![0u8; length];
        reader.read_exact(&mut data)?;
        files.push(String::from_utf8_lossy(&data).into_owned());
    }

    Ok(files)
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: rust-bench <robots_all.bin>");
        std::process::exit(1);
    }

    let files = match load_robots_files(&args[1]) {
        Ok(f) => f,
        Err(e) => {
            eprintln!("Error loading files: {}", e);
            std::process::exit(1);
        }
    };

    let mut matcher = DefaultMatcher::default();
    let mut allowed = 0;

    for content in &files {
        if matcher.one_agent_allowed_by_robots(content, "Googlebot", "http://example.com/") {
            allowed += 1;
        }
    }

    println!("Processed {} files, {} allowed", files.len(), allowed);
}
