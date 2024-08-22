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
                    exit(0);
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
            exit(0);
        } else if !self.options.save {
            self.logger.fatal(String::from("Running the CLI tool without any system commands nor a \"--print\" or \"--save\" flag won't do anything. Use flag \"-h\" or \"--help\" for more information."))
        }
    }
}
