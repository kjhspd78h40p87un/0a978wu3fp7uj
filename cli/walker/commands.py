"""Commands related to interacting with the Walker service.

This primarily serves the purpose of interacting with the Walker service to
generate walks over ICFG labels.
"""

import glog as log

import cli.common.log
import cli.common.uri
import cli.walker.rpc

def walk(service_configuration_handler, input_icfg_uri, output_walks_uri,
         walk_length, walks_per_label):
    """Gets random walks over ICFG labels and saves results to file.

    Args:
        service_configuration_handler: ServicesConfigurationHandler with
            services configured for: Walker.
        input_icfg_uri: String representation of the ICFG Uri to walk over.
        output_walks_uri: String representation of the output walks file.
        walk_length: The length to walk from each starting label.
        walks_per_label: The number of times to perform a random walk from each
            label.
    """
    log.info(f"Beginning walk on {input_icfg_uri}.")

    input_icfg_uri_message = cli.common.uri.parse(input_icfg_uri)
    output_walks_uri_message = cli.common.uri.parse(output_walks_uri)

    # Sending to walker.rpc and waiting for Walker responses.
    cli.walker.rpc.walk(
        walker_stub=service_configuration_handler.get_walker_stub(),
        operations_stub=service_configuration_handler.get_operations_stub(
            "walker"),
        input_icfg_uri=input_icfg_uri_message,
        output_walks_uri=output_walks_uri_message,
        walk_length=walk_length,
        walks_per_label=walks_per_label,
    )

    cli.common.log.log_finished_file_output(
        "Walk", output_walks_uri_message.path)
