use crate::globals::{self, API_URL};
use crate::logger::Logger;
use crate::options::OptionsType;
use colored::Colorize;
use reqwest::{
    blocking::Client,
    header::{HeaderMap, HeaderValue, CONTENT_TYPE},
    StatusCode,
};
use rpassword::prompt_password;

use std::collections::HashMap;
use std::fs;
use std::fs::File;
use std::io::{self, Write};
use std::path::PathBuf;

pub struct Api<'a> {
    request: Client,
    headers: HeaderMap,
    options: &'a mut OptionsType,
    api_key: String,
    data: String,
    status_code: StatusCode,
    save_file_path: PathBuf,
    logger: Logger<'a>,
}

impl<'a> Api<'a> {
    pub fn new(options: &'a mut OptionsType) -> Self {
        let mut logger = Logger::new("Api");
        logger.set_debug(&options.debug);

        let mut headers = HeaderMap::new();
        headers.insert(CONTENT_TYPE, HeaderValue::from_static("text/plain"));

        Api {
            request: Client::new(),
            headers,
            options,
            data: String::new(),
            save_file_path: globals::current_dir().clone(),
            api_key: String::new(),
            status_code: StatusCode::OK,
            logger,
        }
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
            self.logger.fatal(format!("failed to retrieve {req_type} from the nvi API. Expected the response content-type to be text/plain instead received {content_type}."));
        }

        self.data = res.text().unwrap_or_default();

        if !self.status_code.is_success() || self.data.is_empty() {
            self.logger.fatal(format!("failed to retrieve {req_type} from the nvi API. Either the API key was invalid or you haven't created any {req_type} yet!"));
        }

        self.logger
            .debug(format!("successfully retrieved data from {}!", base_url));

        if return_data {
            return self.data.clone();
        }

        self.logger.println(format!(
            "Retrieved the following {req_type} from the nvi API..."
        ));

        let mut options: HashMap<u16, &str> = HashMap::new();
        {
            let mut index: u16 = 1;

            for option in self.data.split("\n") {
                if !option.is_empty() {
                    options.insert(index, option);
                    println!("{}", format!("      [{index}]: {option}").cyan());
                }
                index += 1;
            }

            self.logger.print(
                "Please select one of the options above using the corresponding [number]... "
                    .to_string(),
            );

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

            selection.retain(|c| c.is_ascii_digit());
        }

        let index_option = selection.parse::<u16>().unwrap_or(0);

        if let Some(option) = options.get(&index_option) {
            self.logger.println(format!(
                "The following {req_type} option was selected: \"{option}\"."
            ));

            return option.to_string();
        }

        self.logger.fatal(format!(
            "was provided a(n) invalid {req_type} selection. Aborting.",
        ));
    }

    fn save_envs_to_disk(&mut self) {
        let env_name = format!("{}{}", self.options.environment, ".env");
        if !self.options.dir.is_empty() {
            self.save_file_path.push(&self.options.dir);
        }

        self.save_file_path.push(&env_name);

        if self.save_file_path.is_file() {
            self.logger.println(format!(
                "A file named {:?} already exists at the current path {:?}",
                env_name,
                self.save_file_path.display()
            ));

            self.logger
                .print("Are you sure you want to save and overwrite it? (y|N) ".to_string());
            io::stdout().flush().expect("Failed to flush stdout");

            let mut selection = String::new();
            io::stdin()
                .read_line(&mut selection)
                .expect("Failed to read input");

            if !selection.to_lowercase().contains("y") {
                return;
            }
        }

        let mut env_file = match File::create(&self.save_file_path) {
            Ok(f) => f,
            Err(err) => {
                self.logger.fatal(format!(
                    "was unable to create an ENV file at {:?}. Reason: {}",
                    self.save_file_path.display(),
                    err
                ));
            }
        };

        match env_file.write_all(self.data.as_bytes()) {
            Ok(_) => {
                self.logger.debug(format!(
                    "successfully saved {:?} with API ENVs to disk!",
                    self.save_file_path.display()
                ));
            }
            Err(err) => {
                self.logger.fatal(format!(
                    "was unable to save {}. Reason: {}",
                    self.save_file_path.display(),
                    err
                ));
            }
        };
    }

    pub fn get_and_set_api_envs(&mut self) {
        let mut api_key_file = globals::current_dir().clone();

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
        } else if let Ok(mut ak) = prompt_password(
            "[nvi] Please enter your unique API key: "
                .to_string()
                .cyan(),
        ) {
            ak.retain(|c| c.is_alphanumeric());
            self.api_key.push_str(ak.as_str());
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

        if self.options.save {
            self.save_envs_to_disk();
        }
    }
}
