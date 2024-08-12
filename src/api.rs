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
    options: &'a mut OptionsType,
    api_key: String,
    status_code: reqwest::StatusCode,
    logger: Logger<'a>,
}

impl<'a> Api<'a> {
    pub fn new(options: &'a mut OptionsType) -> Self {
        return Api {
            options,
            api_key: String::new(),
            status_code: reqwest::StatusCode::OK,
            logger: Logger::new("Api"),
        };
    }

    fn fetch_data(
        &mut self,
        req_type: &'a str,
        search_params: Option<String>,
        return_data: bool,
    ) -> String {
        let mut url = format!("{API_URL}/cli/{req_type}/?apiKey={}", self.api_key);
        if let Some(sp) = search_params {
            url.push_str(sp.as_str());
        }

        let res = match reqwest::blocking::get(url) {
            Ok(r) => r,
            Err(err) => self.logger.fatal(format!(
                "Failed to retrieve {req_type} from the nvi API. Reason: {err}.",
            )),
        };

        self.status_code = res.status().to_owned();

        let data = match res.text() {
            Ok(d) => d,
            Err(_) => String::new(),
        };

        if !self.status_code.is_success() | data.is_empty() {
            self.logger.fatal(format!("failed to retrieve {req_type} from the nvi API. Either the API key is invalid or you haven't created any {req_type} yet!"));
        }

        if return_data {
            return data;
        }

        self.logger.println(format!(
            "Retrieved the following {req_type} from the nvi API..."
        ));

        let mut options: HashMap<u16, &str> = HashMap::new();
        {
            let mut index: u16 = 1;

            for opt in data.split("\n") {
                if !opt.is_empty() {
                    options.insert(index, opt);
                    let m = format!("      [{index}]: {opt}");
                    println!("{}", m.cyan());
                }
                index += 1;
            }

            self.logger.print(format!(
                "Please select one of the options above using the corresponding [number]... "
            ));

            io::stdout().flush().expect("Failed to flush stdout");
        }

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
                let option = match options.get(&s) {
                    Some(p) => p,
                    None => self.logger.fatal(format!(
                        "was provided a(n) invalid {req_type} selection. Aborting.",
                    )),
                };

                self.logger.println(format!(
                    "The following {req_type} option was selected: \"{option}\"."
                ));

                return option.to_string();
            }
            Err(_) => self.logger.fatal(format!(
                "was provided a(n) {req_type} invalid selection. Aborting.",
            )),
        };
    }

    pub fn get_and_set_api_envs(&mut self) {
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
                        "could not read file: \"{}\". Reason: {err}.",
                        api_key_file.display(),
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
            if let Ok(mut ak) = prompt_password(msg) {
                ak.retain(|c| c.is_alphanumeric());
                self.api_key.push_str(ak.as_str());
            }
        }

        if self.api_key.is_empty() {
            self.logger.fatal(String::from("was supplied an invalid API key. Please enter a valid API key with aA,zZ,0-9 characters."));
        }

        if self.options.project.is_empty() {
            self.options.project = self.fetch_data("projects", None, false);
        }

        if self.options.environment.is_empty() {
            self.options.environment = self.fetch_data(
                "environments",
                Some(format!("&project={}", self.options.project)),
                false,
            );
        }

        self.options.api_envs = self.fetch_data(
            "secrets",
            Some(format!(
                "&project={}&environment={}",
                self.options.project, self.options.environment
            )),
            true,
        );

        if self.options.debug {
            self.logger
                .debug(format!("set the following options... {:?}", self.options));

            self.logger.debug(format!(
                "successfully retrieved {} project's {} secrets!",
                self.options.project, self.options.environment
            ));
        }
    }
}
