use crate::api::Api;
use crate::arg::ArgParser;
use std::fmt::{Debug, Formatter, Result as FmtResult};

#[derive(Clone, PartialEq)]
pub enum PrintOptions {
    Envs,
    Flags,
    Json,
    Unknown,
}

impl PrintOptions {
    fn as_str(&self) -> &'static str {
        match self {
            PrintOptions::Envs => "envs",
            PrintOptions::Flags => "flags",
            PrintOptions::Json => "json",
            PrintOptions::Unknown => "",
        }
    }

    pub fn to_option(input: String) -> Result<PrintOptions, String> {
        match input.as_str() {
            "json" | "JSON" => Ok(PrintOptions::Json),
            "flags" => Ok(PrintOptions::Flags),
            "envs" | "ENVs" => Ok(PrintOptions::Envs),
            _ => Err(input),
        }
    }
}

impl Debug for PrintOptions {
    fn fmt(&self, f: &mut Formatter) -> FmtResult {
        write!(f, "{:?}", self.as_str())
    }
}

pub struct Options {
    pub api: bool,
    pub api_envs: String,
    pub commands: Vec<String>,
    pub config: String,
    pub debug: bool,
    pub dir: String,
    pub environment: String,
    pub files: Vec<String>,
    pub print: PrintOptions,
    pub project: String,
    pub required_envs: Vec<String>,
    pub save: bool,
}

pub type OptionsType = Options;

impl Debug for Options {
    fn fmt(&self, f: &mut Formatter) -> FmtResult {
        write!(
            f,
            "{{\n    \"api\": {},\n    \"command\": {:?},\n    \"config\": {:?},\n    \"debug\": {},\n    \"directory\": {:?},\n    \"environment\": {:?},\n    \"files\": {:?},\n    \"print\": {:?},\n    \"project\": {:?},\n    \"required_envs\": {:?},\n    \"save\": {}\n}}",
            self.api,
            self.commands,
            self.config,
            self.debug,
            self.dir,
            self.environment,
            self.files,
            self.print,
            self.project,
            self.required_envs,
            self.save
        )
    }
}

impl Options {
    pub fn new(argv: Vec<String>) -> Self {
        let mut options = Options {
            api: false,
            api_envs: String::new(),
            commands: vec![],
            config: String::new(),
            debug: false,
            dir: String::new(),
            environment: String::new(),
            files: vec![String::from(".env")],
            print: PrintOptions::Unknown,
            project: String::new(),
            required_envs: vec![],
            save: false,
        };

        {
            let mut arg_parser = ArgParser::new(&mut options, argv);
            arg_parser.parse();
        }

        {
            if options.api {
                let mut api = Api::new(&mut options);

                api.get_and_set_api_envs();
            }
        }

        options
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::process::{Command, Stdio};

    #[test]
    fn parses_argv() {
        let argv = vec![
            String::from("nvi"),
            String::from("--debug"),
            String::from("--directory"),
            String::from("envs"),
            String::from("--environment"),
            String::from("testing"),
            String::from("--files"),
            String::from("test1.env"),
            String::from("test2.env"),
            String::from("test3.env"),
            String::from("--print"),
            String::from("envs"),
            String::from("--project"),
            String::from("rust"),
            String::from("--required"),
            String::from("TEST1"),
            String::from("TEST2"),
            String::from("TEST3"),
            String::from("--save"),
            String::from("--"),
            String::from("echo"),
            String::from("Hello World!"),
        ];
        let options = Options::new(argv);

        assert_eq!(options.api, false);
        assert_eq!(options.api_envs, String::new());
        assert_eq!(options.commands[0], String::from("echo"));
        assert_eq!(options.commands[1], String::from("Hello World!"));
        assert_eq!(options.debug, true);
        assert_eq!(options.dir, String::from("envs"));
        assert_eq!(options.environment, String::from("testing"));
        assert_eq!(options.files[0], String::from("test1.env"));
        assert_eq!(options.files[1], String::from("test2.env"));
        assert_eq!(options.files[2], String::from("test3.env"));
        assert_eq!(options.project, String::from("rust"));
        assert_eq!(options.print, PrintOptions::Envs);
        assert_eq!(options.required_envs[0], String::from("TEST1"));
        assert_eq!(options.required_envs[1], String::from("TEST2"));
        assert_eq!(options.required_envs[2], String::from("TEST3"));
        assert_eq!(options.save, true);
    }

    #[test]
    fn toml_config() {
        let argv = vec![
            String::from("nvi"),
            String::from("--debug"),
            String::from("--directory"),
            String::from("envs"),
            String::from("--config"),
            String::from("standard"),
        ];
        let options = Options::new(argv);

        assert_eq!(options.api, false);
        assert_eq!(options.api_envs, String::new());
        assert_eq!(options.commands[0], String::from("echo"));
        assert_eq!(options.commands[1], String::from("Hello World!"));
        assert_eq!(options.debug, true);
        assert_eq!(options.dir, String::from("envs"));
        assert_eq!(options.environment, String::from("testing"));
        assert_eq!(options.files[0], String::from("test1.env"));
        assert_eq!(options.files[1], String::from("test2.env"));
        assert_eq!(options.files[2], String::from("test3.env"));
        assert_eq!(options.project, String::from("rust"));
        assert_eq!(options.required_envs[0], String::from("TEST1"));
        assert_eq!(options.required_envs[1], String::from("TEST2"));
        assert_eq!(options.required_envs[2], String::from("TEST3"));
        assert_eq!(options.save, true);
    }

    #[test]
    fn prints_arg_config_error() {
        match Command::new("./target/debug/nvi")
            .args(["--config"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr.contains("The \"--config\" flag must contain an environment name from the nvi.toml configuration file"));
            }
            Err(err) => {
                panic!("Failed to run prints_arg_config_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_arg_directory_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(
                    stderr.contains("The \"--directory\" flag must contain a valid directory path")
                );
            }
            Err(err) => {
                panic!("Failed to run prints_arg_directory_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_arg_environment_error() {
        match Command::new("./target/debug/nvi")
            .args(["--environment"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr
                    .contains("The \"--environment\" flag must contain a valid environment name"));
            }
            Err(err) => {
                panic!("Failed to run prints_arg_environment_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_arg_execute_error() {
        match Command::new("./target/debug/nvi")
            .args(["--"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr
                    .contains("The \"--\" (execute) flag must contain at least 1 system command"));
            }
            Err(err) => {
                panic!("Failed to run prints_arg_execute_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_arg_files_error() {
        match Command::new("./target/debug/nvi")
            .args(["--files"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr.contains("The \"--files\" flag must contain at least 1 .env file"),);
            }
            Err(err) => {
                panic!("Failed to run prints_arg_files_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_arg_print_error() {
        match Command::new("./target/debug/nvi")
            .args(["--print"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr.contains("The \"--print\" flag must contain a valid format"),);
            }
            Err(err) => {
                panic!("Failed to run prints_arg_print_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_arg_print_option_error() {
        match Command::new("./target/debug/nvi")
            .args(["--print", "invalid"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr.contains("The \"--print\" option must be a valid format"));
            }
            Err(err) => {
                panic!("Failed to run prints_arg_print_option_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_arg_help() {
        match Command::new("./target/debug/nvi")
            .args(["--help"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(output.status.success());

                let stdout = String::from_utf8_lossy(&output.stdout);

                assert!(stdout.contains("nvi documentation"));
            }
            Err(err) => {
                panic!("Failed to run prints_arg_help test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_arg_project_error() {
        match Command::new("./target/debug/nvi")
            .args(["--project"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(
                    stderr.contains("The \"--project\" flag must contain a valid project name"),
                );
            }
            Err(err) => {
                panic!("Failed to run prints_arg_project_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_arg_required_error() {
        match Command::new("./target/debug/nvi")
            .args(["--required"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr.contains("The \"--required\" flag must contain at least 1 ENV key"),);
            }
            Err(err) => {
                panic!("Failed to run prints_arg_required_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_selection_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "abc"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr.contains("The environment configuration does't appear to exist"),);
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_selection_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_api_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_api"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(
                    stderr.contains("expected the \"api\" config option to be a boolean value"),
                );
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_api_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_debug_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_debug"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(
                    stderr.contains("expected the \"debug\" config option to be a boolean value"),
                );
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_debug_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_directory_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_directory"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr
                    .contains("expected the \"directory\" config option to be a string value"),);
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_directory_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_environment_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_environment"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr
                    .contains("expected the \"environment\" config option to be a string value"),);
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_environment_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_execute_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_execute"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr.contains("expected the \"execute\" config option to be an array"),);
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_execute_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_execute_array_type_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_execute_2"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr
                    .contains("expected the \"execute\" config option to be an array of strings"),);
            }
            Err(err) => {
                panic!(
                    "Failed to run prints_toml_config_execute_array_type_error test. Reason: {err}"
                );
            }
        }
    }

    #[test]
    fn prints_toml_config_files_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_files"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr.contains("expected the \"files\" config option to be an array"),);
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_files_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_files_array_type_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_files_2"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr
                    .contains("expected the \"files\" config option to be an array of strings"),);
            }
            Err(err) => {
                panic!(
                    "Failed to run prints_toml_config_files_array_type_error test. Reason: {err}"
                );
            }
        }
    }

    #[test]
    fn prints_toml_config_print_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_print"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(
                    stderr.contains("expected the \"print\" config option to be a string value"),
                );
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_print_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_print_option_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_print_2"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr.contains("expected the print config option to be a valid format"),);
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_print_option_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_project_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_project"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(
                    stderr.contains("expected the \"project\" config option to be a string value"),
                );
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_project_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_required_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_required"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr.contains("expected the \"required\" config option to be an array"),);
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_required_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_toml_config_required_array_type_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_required_2"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(stderr
                    .contains("expected the \"required\" config option to be an array of strings"),);
            }
            Err(err) => {
                panic!(
                    "Failed to run prints_toml_config_required_array_type_error test. Reason: {err}"
                );
            }
        }
    }

    #[test]
    fn prints_toml_config_save_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--config", "invalid_save"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(
                    stderr.contains("expected the \"save\" config option to be a boolean value"),
                );
            }
            Err(err) => {
                panic!("Failed to run prints_toml_config_save_error test. Reason: {err}");
            }
        }
    }
}
