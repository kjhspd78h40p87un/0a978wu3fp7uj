"""Classes for gRPC service configuration and handling.

Usage:
    service_handler = ServiceConfigurationHandler(
        eesi_address=eesi_address,
        eesi_port=eesi_port,
        bitcode_address=bitcode_address,
        bitcode_port=bitcode_port,)

    eesi_stub = service_handler.get_eesi_stub()
    bitcode_stub = service_handler.get_bitcode_stub()
"""

import grpc

import proto.bitcode_pb2_grpc
import proto.checker_pb2_grpc
import proto.eesi_pb2_grpc
import proto.embedding_pb2_grpc
import proto.get_graph_pb2_grpc
import proto.operations_pb2_grpc
import proto.walker_pb2_grpc

class ServiceConfigurationHandler:
    """Handles the configuration of services."""

    def __init__(self, eesi_address=None, eesi_port=None,
                 bitcode_address=None, bitcode_port=None,
                 checker_address=None, checker_port=None,
                 embedding_address=None, embedding_port=None,
                 get_graph_address=None, get_graph_port=None,
                 walker_address=None, walker_port=None,
                 max_tasks=4):

        self.eesi_config = self.ServiceConfiguration(
            address=eesi_address, port=eesi_port)
        self.bitcode_config = self.ServiceConfiguration(
            address=bitcode_address, port=bitcode_port)
        self.checker_config = self.ServiceConfiguration(
            address=checker_address, port=checker_port)
        self.embedding_config = self.ServiceConfiguration(
            address=embedding_address, port=embedding_port)
        self.get_graph_config = self.ServiceConfiguration(
            address=get_graph_address, port=get_graph_port)
        self.walker_config = self.ServiceConfiguration(
            address=walker_address, port=walker_port)

        self.max_tasks = max_tasks

    @staticmethod
    def from_args(args):
        """Initializes a ServiceConfigurationHandler from ArgParse args."""
        return ServiceConfigurationHandler(
            eesi_address=args.eesi_address,
            eesi_port=args.eesi_port,
            bitcode_address=args.bitcode_address,
            bitcode_port=args.bitcode_port,
            checker_address=args.checker_address,
            checker_port=args.checker_port,
            embedding_address=args.embedding_address,
            embedding_port=args.embedding_port,
            get_graph_address=args.get_graph_address,
            get_graph_port=args.get_graph_port,
            walker_address=args.walker_address,
            walker_port=args.walker_port,
            max_tasks=args.max_tasks,
        )

    def get_eesi_stub(self):
        """Returns an EESI gRPC stub according to the configuration."""

        return proto.eesi_pb2_grpc.EesiServiceStub(
            self.eesi_config.get_channel())

    def get_bitcode_stub(self):
        """Returns a bitcode gRPC stub according to the configuration."""

        return proto.bitcode_pb2_grpc.BitcodeServiceStub(
            self.bitcode_config.get_channel())

    def get_checker_stub(self):
        """Returns a checker gRPC stub according to the configuration."""

        return proto.checker_pb2_grpc.CheckerServiceStub(
            self.checker_config.get_channel())

    def get_embedding_stub(self):
        """Returns an embedding gRPC stub according to the configuration."""

        return proto.embedding_pb2_grpc.EmbeddingServiceStub(
            self.embedding_config.get_channel())

    def get_get_graph_stub(self):
        """Returns a GetGraph gRPC stub according to the configuration."""

        return proto.get_graph_pb2_grpc.GetGraphServiceStub(
            self.get_graph_config.get_channel())

    def get_walker_stub(self):
        """Returns a Waler gRPC stub according to the configuration."""

        return proto.walker_pb2_grpc.WalkerServiceStub(
            self.walker_config.get_channel())

    def get_operations_stub(self, service):
        """Returns an operation stub according to the supplied flag."""

        services = {"eesi": self.eesi_config,
                    "bitcode": self.bitcode_config,
                    "checker": self.checker_config,
                    "embedding": self.embedding_config,
                    "get_graph": self.get_graph_config,
                    "walker": self.walker_config,}
        if service not in services:
            raise ValueError("Must select a valid choice from: {}".format(
                services.keys()))

        return proto.operations_pb2_grpc.OperationsServiceStub(
            services[service].get_channel())

    class ServiceConfiguration:
        """Represents the configuration for an individual service."""

        def __init__(self, address, port):
            # It is fine to have an empty service configuration, you just
            # can't can't call any of the useful methods.
            # Logical XNOR.
            assert bool(address) is bool(port)

            self.address = address
            self.port = port

        def get_full_address(self):
            """Returns a string for the full address of the service."""

            assert self.address and self.port
            return self.address + ":" + self.port

        def get_channel(self):
            """Returns a gRPC insecure channel according to configuration."""

            assert self.address and self.port
            return grpc.insecure_channel(
                self.get_full_address(),
                options=[
                    ("grpc.max_send_message_length", 100*1024*1024),
                    ("grpc.max_receive_message_length", 100*1024*1024),])
