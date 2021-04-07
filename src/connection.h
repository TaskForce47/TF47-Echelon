#pragma once

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "../protos/greet.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using greet::Greeter;
using greet::HelloReply;
using greet::HelloRequest;

namespace echelon
{
	
	
	class TestClient
	{
	public:
		TestClient(std::shared_ptr<Channel> channel) : stub_(greet::Greeter::NewStub(channel)) {}

		std::string SendPing(const std::string& message) {
			HelloRequest request;
			request.set_name(message);

			HelloReply reply;
			ClientContext context;
			
			Status status = stub_->SayHello(&context, request, &reply);

			if (status.ok()) {
				return reply.message();
			}
			else 
			{
				std::stringstream ss;
				ss << status.error_code() << ": " << status.error_message() << std::endl;
				throw ss.str();
				return "RPC failed";
			}
		}

	private:
		std::unique_ptr<greet::Greeter::Stub> stub_;
	};
}
