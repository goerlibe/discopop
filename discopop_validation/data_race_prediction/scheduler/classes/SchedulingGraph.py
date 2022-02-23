import warnings
import random
import string

from typing import Tuple, List

from discopop_validation.data_race_prediction.behavior_modeller.classes.BehaviorModel import BehaviorModel
from discopop_validation.data_race_prediction.scheduler.classes.ScheduleElement import ScheduleElement
import networkx as nx  # type:ignore
import matplotlib.pyplot as plt  # type:ignore

from networkx.drawing.nx_agraph import graphviz_layout  # type:ignore


class SchedulingGraph(object):
    graph: nx.DiGraph
    root_node_identifier: Tuple[Tuple, int]
    lock_names: List[str] = []
    var_names: List[str] = []
    dimensions: List[int]
    fingerprint: str
    def __init__(self, dim: List[int], behavior_models: List[BehaviorModel]):
        self.dimensions = dim
        self.fingerprint = ''.join(random.choices(string.ascii_uppercase + string.digits, k=12))
        self.graph = nx.DiGraph()
        # add root node, id = (tuple of n zero´s, last executed thread id)
        self.root_node_identifier = (tuple(0 for _ in range(len(dim))), -1, self.fingerprint)
        self.graph.add_node((tuple(0 for _ in range(len(dim))), -1, self.fingerprint), data=None)
        self.__old_add_nodes_rec((tuple(0 for _ in range(len(dim))), -1, self.fingerprint), dim.copy(), behavior_models)
        # determine lock and var names
        for behavior_model in behavior_models:
            for schedule_element in behavior_model.schedule_elements:
                self.lock_names += schedule_element.lock_names
                self.lock_names = list(set(self.lock_names))
                self.var_names += schedule_element.var_names
                self.var_names = list(set(self.var_names))

    def plot_graph(self):
        plt.subplot(121)
        pos = nx.fruchterman_reingold_layout(self.graph)
        nx.draw(self.graph, pos, with_labels=False, arrows=True, font_weight='bold')
        labels = {}
        for node in self.graph.nodes:
            labels[node] = str(node) + "\n" + str(self.graph.nodes[node]["data"])
        nx.draw_networkx_labels(self.graph, pos, labels)
        plt.show()


    def __old_add_nodes_rec(self, parent_node_identifier, dim, behavior_models: List[BehaviorModel]):
        for i in range(len(dim)):
            if dim[i] <= 0:
                continue
            parent_node_id, parent_last_thread_id, fingerprint = parent_node_identifier
            dim_copy = dim.copy()
            dim_copy[i] -= 1
            # add new node if not already contained in graph
            new_node_id = list(parent_node_id)
            new_node_id[i] += 1
            new_node_id_tuple = tuple(new_node_id)
            # check for root node
            if self.graph.nodes[parent_node_identifier]["data"] is None:
                # root node
                last_thread_id = -1
            else:
                # not root node
                last_thread_id = self.graph.nodes[parent_node_identifier]["data"].thread_id
            new_node_identifier = (new_node_id_tuple, last_thread_id, self.fingerprint)
            if new_node_identifier not in self.graph.nodes:
                # update thread id
                self.graph.add_node(new_node_identifier, data=behavior_models[i].schedule_elements[new_node_id_tuple[i] - 1])

            # add edge from parent_node_id to new_node_id
            if not (parent_node_identifier, new_node_identifier) in self.graph.edges:
                self.graph.add_edge(parent_node_identifier, new_node_identifier)
            # start recursion
            self.__old_add_nodes_rec(new_node_identifier, dim_copy, behavior_models)

    def get_leaf_node_identifiers(self):
        leaf_node_identifiers = []
        for node in self.graph.nodes:
            if len(self.graph.out_edges(node)) == 0:
                leaf_node_identifiers.append(node)
        return leaf_node_identifiers

    def get_root_node_identifier(self):
        return self.root_node_identifier


    def sequential_compose(self, other_graph):
        """add edges between leaf nodes of this and root node of other_graph."""
        # new dimensions are the component-wise maximum of both
        new_dimensions = []
        while len(self.dimensions) > 0 and len(other_graph.dimensions) > 0:
            new_dimensions.append(max(self.dimensions.pop(0), other_graph.dimensions.pop(0)))
        if len(self.dimensions) > 0:
            new_dimensions += self.dimensions
        elif len(other_graph.dimensions) > 0:
            new_dimensions += other_graph.dimensions
        self.dimensions = new_dimensions

        leaf_node_ids_buffer = self.get_leaf_node_identifiers()
        self.graph.add_nodes_from(other_graph.graph.nodes(data=True))
        self.graph.add_edges_from(other_graph.graph.edges(data=True))
        for leaf_node_id in leaf_node_ids_buffer:
            self.graph.add_edge(leaf_node_id, other_graph.get_root_node_identifier())

        self.lock_names += [ln for ln in other_graph.lock_names if ln not in self.lock_names]
        self.var_names += [vn for vn in other_graph.var_names if vn not in self.var_names]


    def parallel_compose(self, other_graph):
        # new dimensions are the component-wise maximum of both
        new_dimensions = []
        while len(self.dimensions) > 0 and len(other_graph.dimensions) > 0:
            new_dimensions.append(max(self.dimensions.pop(0), other_graph.dimensions.pop(0)))
        if len(self.dimensions) > 0:
            new_dimensions += self.dimensions
        elif len(other_graph.dimensions) > 0:
            new_dimensions += other_graph.dimensions
        self.dimensions = new_dimensions


        print("PARALLEL COMPOSE")
        warnings.warn("TODO")