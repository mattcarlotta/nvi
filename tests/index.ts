if (process.env.DISABLE_LOG === 'true') {
    console.log = () => {};
}
import './config.spec';
import './parser.spec';
