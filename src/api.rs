use crate::globals::API_URL;
use crate::logger::Logger;
use crate::options::OptionsType;
use colored::Colorize;
use reqwest::{
    blocking::Client,
    header::{HeaderMap, HeaderValue, CONTENT_TYPE},
};
use rpassword::prompt_password;

use std::collections::HashMap;
use std::env;
use std::fs;
use std::io::{self, Write};
use std::path::PathBuf;

pub struct Api<'a> {
    request: Client,
    headers: HeaderMap,
    options: &'a mut OptionsType,
    api_key: String,
    status_code: reqwest::StatusCode,
    logger: Logger<'a>,
}

impl<'a> Api<'a> {
    pub fn new(options: &'a mut OptionsType) -> Self {
        let mut logger = Logger::new("Api");
        logger.set_debug(&options.debug);

        let mut headers = HeaderMap::new();
        headers.insert(CONTENT_TYPE, HeaderValue::from_static("text/plain"));

        return Api {
            request: Client::new(),
            headers,
            options,
            api_key: String::new(),
            status_code: reqwest::StatusCode::OK,
            logger,
        };
    }

    fn fetch_data(
        &mut self,
        req_type: &'a str,
        search_params: Option<String>,
        return_data: bool,
    ) -> String {
        let base_url = format!("{API_URL}/cli/{req_type}");
        let mut url = format!("{base_url}/?apiKey={}", self.api_key);
        if let Some(sp) = search_params {
            url.push_str(sp.as_str());
        }

        self.logger
            .debug(format!("is attempting to get data from {}...", base_url));

        let res = match self.request.get(url).headers(self.headers.clone()).send() {
            Ok(r) => r,
            Err(err) => self.logger.fatal(format!(
                "failed to retrieve {req_type} from the nvi API. Reason: {err}.",
            )),
        };

        self.status_code = res.status().to_owned();

        let mut content_type = "";
        if let Some(ctv) = res.headers().get(CONTENT_TYPE) {
            if let Ok(ct) = ctv.to_str() {
                content_type = ct;
            }
        };

        if content_type != "text/plain; charset=utf-8" {
            self.logger.fatal(format!("failed to retrieve {req_type} from the nvi API. Excepted the response content-type to be text/plain instead received {content_type}."));
        }

        let data = res.text().unwrap_or(String::new());

        if !self.status_code.is_success() || data.is_empty() {
            self.logger.fatal(format!("failed to retrieve {req_type} from the nvi API. Either the API key was invalid or you haven't created any {req_type} yet!"));
        }

        self.logger
            .debug(format!("successfully retrieved data from {}!", base_url));

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
                    println!("{}", format!("      [{index}]: {opt}").cyan());
                }
                index += 1;
            }

            self.logger.print(format!(
                "Please select one of the options above using the corresponding [number]... "
            ));

            io::stdout().flush().expect("Failed to flush stdout");
        }

        if options.is_empty() {
            self.logger.fatal(format!(
                "failed to parse {req_type} options from the nvi API. Aborting."
            ));
        }

        let mut selection = String::new();
        {
            let stdin = std::io::stdin();
            stdin
                .read_line(&mut selection)
                .expect("Failed to read line");

            selection.retain(|c| c.is_digit(10));
        }

        let index_option = selection.parse::<u16>().unwrap_or(0);

        if let Some(option) = options.get(&index_option) {
            self.logger.println(format!(
                "The following {req_type} option was selected: \"{option}\"."
            ));

            return option.to_string();
        } else {
            self.logger.fatal(format!(
                "was provided a(n) invalid {req_type} selection. Aborting.",
            ));
        };
    }

    pub fn get_and_set_api_envs(&mut self) {
        let mut api_key_file = env::current_dir().unwrap_or(PathBuf::new());

        if !self.options.dir.is_empty() {
            api_key_file.push(&self.options.dir);
        }

        api_key_file.push(".nvi-key");

        if api_key_file.is_file() {
            match fs::read_to_string(&api_key_file) {
                Ok(mut fc) => {
                    fc.retain(|c| c.is_alphanumeric());
                    self.api_key = fc;
                }
                Err(err) => {
                    self.logger.fatal(format!(
                        "could not read file: \"{}\". Reason: {err}.",
                        api_key_file.display(),
                    ));
                }
            };

            self.logger.debug(format!(
                "successfully parsed the API key found within \"{}\"!",
                api_key_file.display()
            ));
        } else {
            if let Ok(mut ak) =
                prompt_password(format!("[nvi] Please enter your unique API key: ").cyan())
            {
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

        self.logger.debug(format!(
            "has set the following options... \n{:#?}",
            self.options
        ));

        self.logger.debug(format!(
            "successfully retrieved the \"{}\" project's \"{}\" environment secrets!",
            self.options.project, self.options.environment
        ));
    }
}
