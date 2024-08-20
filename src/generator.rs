use crate::logger::Logger;
use crate::options::OptionsType;
use std::collections::HashMap;
use std::process::{exit, Command, Stdio};

pub struct Generator<'a> {
    options: &'a OptionsType,
    envs: &'a HashMap<String, String>,
    logger: Logger<'a>,
}

impl<'a> Generator<'a> {
    pub fn new(options: &'a OptionsType, envs: &'a HashMap<String, String>) -> Self {
        let mut logger = Logger::new("Generator");
        logger.set_debug(&options.debug);

        return Generator {
            envs,
            options,
            logger,
        };
    }

    pub fn run(self) {
        if !self.options.commands.is_empty() {
            match Command::new(&self.options.commands[0])
                .args(&self.options.commands[1..self.options.commands.len()])
                .envs(self.envs)
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
        } else if !self.options.print {
            // TODO: print ENVS
            exit(0);
        }

        self.logger.fatal(String::from("Running the CLI tool without any system commands nor a \"--print\" or \"--save\" flag won't do anything. Use flag \"-h\" or \"--help\" for more information."))
    }
}
