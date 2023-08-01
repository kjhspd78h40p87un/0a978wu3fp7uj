"""The Walker Python command line interface.
"""

import grpc
import proto.get_graph_pb2
import proto.walker_pb2
import proto.walker_pb2_grpc
import proto.operations_pb2
import proto.operations_pb2_grpc
import time
import argparse
import os

def random_walk(walker_server_address, icfg_file_path, num_walks_per_label, walk_length):
    icfg_file_absolute_path = os.path.abspath(icfg_file_path)

    walker_channel = grpc.insecure_channel(walker_server_address)

    input_icfg_uri = proto.operations_pb2.Uri(
        scheme="file",
        path=icfg_file_absolute_path,
    )

    stub = proto.walker_pb2_grpc.WalkerServiceStub(walker_channel)
    random_walk_request = proto.walker_pb2.RandomWalkLegacyIcfgRequest(
        input_icfg=input_icfg_uri,
        walk_length=100,
        number_walks=100,
    )

    random_walk_response = stub.RandomWalkLegacyIcfg(random_walk_request)

    max_walk_length = 0
    total_walk_length = 0
    number_of_sentences = 0
    for sentence in random_walk_response:
        if len(sentence.words) > max_walk_length:
            max_walk_length = len(sentence.words)
        total_walk_length += len(sentence.words) 
        number_of_sentences += 1

        words = " ".join([word.label for word in sentence.words])
        #print(words)

    print("Number of sentences: {}".format(number_of_sentences))
    print("Maximum walk length: {}".format(max_walk_length)) 
    print("Average walk length: {}".format(float(total_walk_length) / float(number_of_sentences)))

    return None

def random_walk_bg(walker_server_address, icfg_file_path, num_walks, walk_length):
    icfg_file_absolute_path = os.path.abspath(icfg_file_path)

    walker_channel = grpc.insecure_channel(walker_server_address)

    input_icfg_uri = proto.operations_pb2.Uri(
        scheme="file",
        path=icfg_file_absolute_path,
    )
    output_walks_uri = proto.operations_pb2.Uri(
        scheme="file",
        path="/tmp/test.walks",
    )

    stub = proto.walker_pb2_grpc.WalkerServiceStub(walker_channel)
    random_walk_request = proto.walker_pb2.RandomWalkLegacyIcfgRequest(
        input_icfg=input_icfg_uri,
        output_walks=output_walks_uri,
        walk_length=100,
        number_walks=100,
    )

    random_walk_response = stub.RandomWalkLegacyIcfgBackground(random_walk_request)

    operations_stub = proto.operations_pb2_grpc.OperationsServiceStub(walker_channel)
    finished_response = wait_for_operation(operations_stub, random_walk_response.name)

    print(finished_response)

def wait_for_operation(operations_stub, operation_name):
    """Busy waits for an operation.
    Returns the operation response that has done set to true.
    """
    done = False
    while not done:
        try:
            get_operation_req = proto.operations_pb2.GetOperationRequest(
                name=operation_name
            )
            get_operation_response = operations_stub.GetOperation(
                get_operation_req)
            done = get_operation_response.done
            time.sleep(1)
        except Exception as e:
            print(e)
            time.sleep(1)

    return get_operation_response

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--walker-server', help='Address to the walker server in HOST:PORT format.'
    )

    # TODO: Create icfg service and modify this.
    parser.add_argument(
        '--icfg', help="Path to icfg file", required=True)
    parser.add_argument('--num-walks',
                        help="The number of walks to perform for each label.")
    parser.add_argument('--walk-length',
                        help="Length of each walk.")
    args = parser.parse_args()

    if args.walker_server:
        walker_server = args.walker_server
    else:
        walker_server = "localhost:50055"

    random_walk_bg(walker_server, 
        args.icfg, args.num_walks, args.walk_length)
