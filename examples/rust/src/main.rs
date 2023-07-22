fn get_env(key: &str) -> String {
    if let Ok(env_value) = std::env::var(key) {
        return env_value;
    } else {
        return String::new();
    }
}

fn main() {
    assert_eq!(get_env("BASIC_ENV"), "true");
    assert_eq!(get_env("QUOTES"), "sad\"wow\"bak");
    assert_eq!(
        get_env("MULTI_LINE_KEY"),
        "ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com"
    );
    assert_eq!(
        get_env("MONGOLAB_URI"),
        "mongodb://root:password@abcd1234.mongolab.com:12345/localhost"
    );

    println!("Successfully assigned ENVs to the process!");
    std::process::exit(0);
}
