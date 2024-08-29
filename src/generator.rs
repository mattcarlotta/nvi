use crate::logger::Logger;
use crate::options::{OptionsType, PrintOptions};
use std::collections::HashMap;
use std::process::{exit, Command};

pub struct Generator<'a> {
    options: &'a OptionsType,
    output: String,
    logger: Logger<'a>,
}

impl<'a> Generator<'a> {
    pub fn new(options: &'a OptionsType) -> Self {
        let mut logger = Logger::new("Generator");
        logger.set_debug(&options.debug);

        return Generator {
            options,
            output: String::new(),
            logger,
        };
    }

    pub fn run(&mut self, envs: HashMap<String, String>) {
        if !self.options.commands.is_empty() {
            let mut child_process = Command::new(&self.options.commands[0])
                .args(&self.options.commands[1..self.options.commands.len()])
                .envs(&envs)
                .spawn()
                .expect("Failed to execute command");

            child_process.wait().expect("Failed to read command output");
        } else if self.options.print != PrintOptions::Unknown {
            let mut env_count = 1;
            if self.options.print == PrintOptions::JSON {
                self.output.push_str("{");
            }

            for (key, value) in &envs {
                let kv: String;
                match self.options.print {
                    PrintOptions::JSON => {
                        kv = format!("{:?}:{:?},", key, value);
                    }
                    PrintOptions::Flags => {
                        kv = format!("--{} {} ", key, value);
                    }
                    _ => {
                        kv = format!("{}={}\n", key, value);
                    }
                }
                let mut kvl = kv.len();
                if env_count == envs.len() {
                    kvl -= 1;
                }
                self.output.push_str(&kv[0..kvl]);
                env_count += 1;
            }

            if self.options.print == PrintOptions::JSON {
                self.output.push_str("}");
            }
            println!("{}", self.output);
        } else if !self.options.api && !self.options.save {
            self.logger.fatal(String::from("encountered an error. Running nvi without any of these flags: \"--print,\" \"--api with --save,\" or \"--\" won't result in any meaningful action. Use flag \"--help\" for more information."))
        }

        exit(0);
    }
}

#[cfg(test)]
mod tests {
    use std::process::{Command, Stdio};

    #[test]
    fn prints_envs_as_json() {
        match Command::new("./target/debug/nvi")
            .args([
                "--print",
                "json",
                "--directory",
                "envs",
                "--files",
                "base.env",
            ])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(output.status.success());

                let stdout = String::from_utf8_lossy(&output.stdout);

                assert_eq!(stdout, String::from("{\"BASE\":\"hello\"}\n"));
            }
            Err(err) => {
                panic!("Failed to run prints_envs_as_json test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_envs_as_flags() {
        match Command::new("./target/debug/nvi")
            .args([
                "--print",
                "flags",
                "--directory",
                "envs",
                "--files",
                "base.env",
            ])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(output.status.success());

                let stdout = String::from_utf8_lossy(&output.stdout);

                assert_eq!(stdout, String::from("--BASE hello\n"));
            }
            Err(err) => {
                panic!("Failed to run prints_envs_as_flags test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_envs() {
        match Command::new("./target/debug/nvi")
            .args([
                "--print",
                "envs",
                "--directory",
                "envs",
                "--files",
                "base.env",
            ])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(output.status.success());

                let stdout = String::from_utf8_lossy(&output.stdout);

                assert_eq!(stdout, String::from("BASE=hello\n"));
            }
            Err(err) => {
                panic!("Failed to run prints_envs test. Reason: {err}");
            }
        }
    }

    #[test]
    fn assigns_envs_to_command() {
        match Command::new("./target/debug/nvi")
            .args([
                "--directory",
                "envs",
                "--files",
                "message.env",
                "--",
                "printenv",
                "MESSAGE",
            ])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(output.status.success());

                let stdout = String::from_utf8_lossy(&output.stdout);

                assert_eq!(stdout, String::from("It works on my machine.\n"));
            }
            Err(err) => {
                panic!("Failed to run assigns_envs_to_command test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr.contains("Generator encountered an error"));
            }
            Err(err) => {
                panic!("Failed to run prints_error test. Reason: {err}");
            }
        }
    }
}
