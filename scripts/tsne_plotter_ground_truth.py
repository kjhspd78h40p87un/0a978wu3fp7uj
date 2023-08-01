import argparse
from collections import defaultdict
from gensim.models.keyedvectors import KeyedVectors
import matplotlib.pyplot as plt
import pymongo
from sklearn.manifold import TSNE
import numpy as np

import os


def main():
    command, commands = _parse_args()
    model = KeyedVectors.load_word2vec_format(commands["model"], binary=False)
    specifications = _get_specifications(commands["ground_truth_path"])
    command(
        model=model, 
        specifications=specifications,
        project_title=commands["project_title"],
        output_file=commands["output_file"],
        perplexity=commands["perplexity"],
        learning_rate=commands["learning_rate"]
    )
    return
    
def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", required=True)
    parser.add_argument("--ground-truth-path", required=True)
    parser.add_argument("--perplexity", default=5, type=int)
    parser.add_argument("--learning-rate", default=250, type=int)
    parser.add_argument("--output-file", required=True)

    args = parser.parse_args()

    command = _plot_tsne
    commands = {}
    commands["model"] = args.model
    commands["model"] = args.model
    commands["ground_truth_path"] = args.ground_truth_path
    commands["project_title"] = os.path.basename(args.ground_truth_path)
    commands["perplexity"] = args.perplexity
    commands["learning_rate"] = args.learning_rate
    commands["output_file"] = args.output_file
    
    return command, commands
    
def _plot_tsne(model, specifications, project_title, output_file, perplexity, learning_rate):
    class_to_labels = defaultdict(list)
    for function in model.index_to_key:
        # Not the best way to handle this, but for now this works for
        # labels that are not functions.
        if function.startswith("F2V"):
            continue
        if function in specifications.keys():
            lattice_element_string = specifications[function]
            class_to_labels[lattice_element_string].append(function)
        else:
            continue

    X = []
    boundaries = []
    row = 0
    row_to_label = []

    for label_class in class_to_labels.keys():
        for label in class_to_labels[label_class]:
            X.append(model[label])
            row_to_label.append(label)
            row += 1
        boundaries.append([label_class, row])

    # No randomness for now, since we want comparable TSNE plots.
    tsne = TSNE(n_components=2, perplexity=perplexity, init="pca", random_state=123, learning_rate=learning_rate, n_iter=5000) 
    X_tsne = tsne.fit_transform(np.asarray(X))

    plt.style.use("ggplot")
    #plt.rcParams['aces.edegecolor'] = '#777777'
    #plt.rcParams['aces.facecolor'] = '#FFFFFF'
    fig, ax = plt.subplots()

    ax.set_xticks([])
    ax.set_yticks([])

    to_legend = ['==0', '!=0', '>0', '<0', '<=0', '>=0', 'emptyset', 'top']

    prev_boundary = 0
    legend = []
    legend_names = []
    event_label_lookup = {}
    marker_size = 16
    for boundary in boundaries:
        next_boundary = boundary[1]
        class_results_x = X_tsne[prev_boundary:next_boundary, 0]
        class_results_y = X_tsne[prev_boundary:next_boundary, 1]

        if boundary[0] == "emptyset":
            class_results_scatter = ax.scatter(class_results_x, class_results_y,
                                               marker='o', color='grey', 
                                               s=marker_size)
        elif boundary[0] == "top":
            class_results_scatter = ax.scatter(class_results_x, class_results_y, 
                                               marker='p', color='white', 
                                               s=marker_size)
        elif boundary[0] == "==0":
            class_results_scatter = ax.scatter(class_results_x, class_results_y,
                                               marker='o', color='blue', 
                                               s=marker_size)
        elif boundary[0] == "!=0":
            class_results_scatter = ax.scatter(class_results_x, class_results_y, 
                                               marker='v', color='green', 
                                               s=marker_size)
        elif boundary[0] == ">0":
            class_results_scatter = ax.scatter(class_results_x, class_results_y, 
                                               marker='^', color='yellow', 
                                               s=marker_size)
        elif boundary[0] == "<0":
            class_results_scatter = ax.scatter(class_results_x, class_results_y,
                                               marker='<', color='purple', 
                                               s=marker_size)
        elif boundary[0] == "<=0":
            class_results_scatter = ax.scatter(class_results_x, class_results_y, 
                                               marker='<', color='orange', 
                                               s=marker_size)
        elif boundary[0] == ">=0":
            class_results_scatter = ax.scatter(class_results_x, class_results_y, 
                                               marker='8', color='green', 
                                               s=marker_size)
        else:
            raise Exception(f"Unexpected class label {boundary[0]} found")
    
        event_label_lookup[class_results_scatter] = row_to_label[prev_boundary:next_boundary]
        if boundary[0] in to_legend:
            legend.append(class_results_scatter)
            if boundary[0] == "emptyset" or boundary[0] == "top":
                if boundary[0] == "emptyset":
                    legend_names.append(r"$\emptyset$")
                if boundary[0] == "top":
                    legend_names.append(r"$\top$")
            else:
                legend_names.append(boundary[0])
        prev_boundary = next_boundary

    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])

    legend = tuple(legend)
    leg = plt.legend(
        legend,
        legend_names,
        scatterpoints=1,
        loc='lower left',
        ncol=1,
        fontsize=12,
        frameon=True,
        bbox_to_anchor=(1.01, 0.3),
    )
    leg.get_frame().set_edgecolor('black')
    leg.get_frame().set_facecolor('none')

    #plt.title("TSNE projection for Pidgin functions classified by specification.")
    
 
    if output_file:
        plt.savefig(output_file, format="png", dpi=150, bbox_inches="tight")
    else:
        plt.tight_layout()
        plt.show()

def _get_specifications(ground_truth_path):
    specifications = {}

    with open(ground_truth_path) as f:
        for line in f:
            split_line = line.split()
            if len(split_line) < 2 or (len(split_line) == 2 and split_line[1] == "X"):
                continue
            specifications[split_line[0]] = split_line[1]

    return specifications

if __name__ == "__main__":
	main()
