use crate::logger::Logger;
use crate::options::OptionsType;
use std::collections::HashMap;
use std::process::{exit, Command, Stdio};

pub struct Generator<'a> {
    options: OptionsType,
    envs: HashMap<String, String>,
    logger: Logger<'a>,
}

impl<'a> Generator<'a> {
    pub fn new(options: OptionsType, envs: HashMap<String, String>) -> Self {
        let mut logger = Logger::new("Generator");
        logger.set_debug(&options.debug);

        return Generator {
            envs,
            options,
            logger,
        };
    }

    pub fn run(&self) {
        if !self.options.commands.is_empty() {
            match Command::new(&self.options.commands[0])
                .args(&self.options.commands[1..self.options.commands.len()])
                .envs(&self.envs)
                .stdin(Stdio::null())
                .stdout(Stdio::piped())
                .stderr(Stdio::piped())
                .spawn()
            {
                Ok(c) => {
                    let output = c.wait_with_output().expect("Failed to read command output");

                    if !output.status.success() {
                        self.logger.fatal(format!(
                            "was unable to run: {:?}\n{}\n{}",
                            &self.options.commands.join(" "),
                            output.status,
                            String::from_utf8_lossy(&output.stderr)
                        ));
                    }

                    println!("{}", String::from_utf8_lossy(&output.stdout));
                }
                Err(err) => {
                    self.logger.fatal(format!(
                        "was unable to run: {}. Reason: {}",
                        &self.options.commands.join(" "),
                        err
                    ));
                }
            }
        } else if self.options.print {
            let mut env_json = String::new();
            let mut env_count = 1;
            for (key, value) in &self.envs {
                let mut kv = format!("{:?}:{:?},", key, value);
                if env_count == self.envs.len() {
                    kv.pop();
                }
                env_json.push_str(kv.as_str());
                env_count += 1;
            }

            println!("{{{}}}", env_json);
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
    fn prints_envs() {
        match Command::new("./target/debug/nvi")
            .args(["--print", "--directory", "envs", "--files", "base.env"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert_eq!(output.status.success(), true);

                let stdout = String::from_utf8_lossy(&output.stdout);

                assert_eq!(stdout, String::from("{\"BASE\":\"hello\"}\n"));
            }
            Err(err) => {
                panic!("Failed to run print_envs test. Reason: {err}");
            }
        }
    }

    #[test]
    fn assigns_envs_to_command() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--", "echo", "Hello", "World!"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert_eq!(output.status.success(), true);

                let stdout = String::from_utf8_lossy(&output.stdout);

                assert_eq!(stdout, String::from("Hello World!\n\n"));
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

                assert_eq!(output.status.success(), false);

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert_eq!(stderr.contains("Generator encountered an error"), true);
            }
            Err(err) => {
                panic!("Failed to run prints_error test. Reason: {err}");
            }
        }
    }
}
