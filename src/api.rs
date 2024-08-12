use crate::globals::API_URL;
use crate::logger::Logger;
use crate::options::OptionsType;
use colored::*;
use reqwest;
use rpassword::prompt_password;

use std::collections::HashMap;
use std::env;
use std::fs;
use std::io::{self, Write};
use std::path::PathBuf;

pub struct Api<'a> {
    options: &'a OptionsType,
    api_key: String,
    // env_file_path: String,
    status_code: reqwest::StatusCode,
    logger: Logger<'a>,
}

impl<'a> Api<'a> {
    pub fn new(options: &'a OptionsType) -> Self {
        return Api {
            options,
            api_key: String::new(),
            // env_file_path: String::new(),
            status_code: reqwest::StatusCode::OK,
            logger: Logger::new("Api"),
        };
    }

    pub fn get_key_from_file_or_input(&mut self) -> &mut Self {
        let mut api_key_file = match env::current_dir() {
            Ok(p) => p,
            Err(_) => PathBuf::new(),
        };

        if !self.options.dir.is_empty() {
            api_key_file.push(&self.options.dir);
        }

        api_key_file.push(".nvi-key");

        if api_key_file.is_file() {
            self.api_key = match fs::read_to_string(&api_key_file) {
                Ok(mut fc) => {
                    fc.retain(|c| c.is_alphanumeric());
                    fc
                }
                Err(err) => {
                    self.logger.fatal(format!(
                        "could not read file: \"{}\". Reason: {}.",
                        api_key_file.display(),
                        err
                    ));
                }
            };

            if self.options.debug {
                self.logger.debug(format!(
                    "successfully read API key from {}!",
                    api_key_file.display()
                ));
            }
        } else {
            let msg = format!("[nvi] Please enter your unique API key: ").cyan();
            self.api_key = match prompt_password(msg) {
                Ok(mut ak) => {
                    ak.retain(|c| c.is_alphanumeric());
                    ak
                }
                Err(_) => String::new(),
            };
        }

        if self.api_key.is_empty() {
            self.logger.fatal(String::from("was supplied an invalid API key. Please enter a valid API key with aA,zZ,0-9 characters."));
        }

        if self.options.project.is_empty() {
            let res = match reqwest::blocking::get(format!(
                "{}/cli/projects/?apiKey={}",
                API_URL, self.api_key
            )) {
                Ok(r) => r,
                Err(e) => self.logger.fatal(format!(
                    "Failed to retrieve projects from API. Reason: {}.",
                    e
                )),
            };

            self.status_code = res.status().to_owned();

            let text = match res.text() {
                Ok(t) => t,
                Err(_) => String::new(),
            };

            if !self.status_code.is_success() | text.is_empty() {
                self.logger.fatal(String::from("failed to retrieve projects. Either the API key is invalid or you haven't created any projects yet!"));
            }

            self.logger.println(format!(
                "Retrieved the following projects from the nvi API..."
            ));
            let mut projects: HashMap<u16, &str> = HashMap::new();
            let mut index: u16 = 1;

            for p in text.split("\n") {
                if !p.is_empty() {
                    projects.insert(index, p);
                    let m = format!("      [{}]: {}", index, p);
                    println!("{}", m.cyan());
                }
                index += 1;
            }

            self.logger.print(format!(
                "Please select one of the options above using the corresponding [number]... "
            ));
            io::stdout().flush().expect("Failed to flush stdout");

            let mut selection = String::new();

            {
                let stdin = std::io::stdin();
                stdin
                    .read_line(&mut selection)
                    .expect("Failed to read line");

                selection.retain(|c| c.is_digit(10));
            }

            match selection.parse::<u16>() {
                Ok(s) => {
                    let project = match projects.get(&s) {
                        Some(p) => p,
                        None => self
                            .logger
                            .fatal(String::from("was provided an valid selection. Aborting.")),
                    };

                    self.logger.println(format!(
                        "The following project was selected: \"{}\".",
                        project
                    ));
                }
                Err(_) => self
                    .logger
                    .fatal(String::from("was provided an valid selection. Aborting.")),
            };
        }

        if self.options.environment.is_empty() {
            // TODO: Implement fetching environments from API
        }

        return self;
    }
}
