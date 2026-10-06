fn main() -> Result<(), Box<dyn std::error::Error>> {
    build_protos()?;
    Ok(())
}

fn build_protos() -> Result<(), Box<dyn std::error::Error>> {
    if std::env::var_os("PROTOC").is_none() {
        unsafe { std::env::set_var("PROTOC", protoc_bin_vendored::protoc_bin_path()?) };
    }

    tonic_prost_build::configure()
        .build_server(true)
        .build_client(true)
        .compile_protos(&["proto/numstore/v1/numstore.proto"], &["proto"])?;

    println!("cargo:rerun-if-changed=proto");
    Ok(())
}
