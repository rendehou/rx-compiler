use std::{env, error::Error, fs, process};

use antlr4rust::{
    common_token_stream::CommonTokenStream,
    tree::ParseTree,
    InputStream,
};

#[allow(dead_code)]
mod generated;

fn main() {
    if let Err(error) = run() {
        eprintln!("rx-antlr-cst: {error}");
        process::exit(1);
    }
}

fn run() -> Result<(), Box<dyn Error>> {
    let input_path = env::args_os()
        .nth(1)
        .unwrap_or_else(|| "samples/minimal.rx".into());
    let source = fs::read_to_string(&input_path)?;

    let lexer = generated::rxlexer::RxLexer::new(InputStream::new(source.as_str()));
    let mut parser = generated::rxparser::RxParser::new(CommonTokenStream::new(lexer));
    let tree = parser.crate_()?;

    println!("{}", tree.to_string_tree(&*parser));
    Ok(())
}
