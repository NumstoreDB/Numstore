use nsserver::proto::num_store_server::NumStoreServer;
use nsserver::NumStoreService;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let addr = "127.0.0.1:50051".parse()?;
    println!("nsserver listening on {addr}");
    tonic::transport::Server::builder()
        .add_service(NumStoreServer::new(NumStoreService))
        .serve(addr)
        .await?;
    Ok(())
}
