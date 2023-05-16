if (process.env.DISABLE_LOG === 'true') {
    console.log = () => { };
}
import "./config.spec";
import "./loadEnvConfig.spec";
import "./readEnvFile.spec";
