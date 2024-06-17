/**
 * Copyright (c) Institut national de l'information géographique et forestière https://www.ign.fr/
 *
 * File main authors:
 *  - JM Muller
 *
 * This file is part of Comp3D: https://github.com/IGNF/Comp3D
 *
 * Comp3D is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Comp3D is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Comp3D. If not, see <https://www.gnu.org/licenses/>.
 */

#include "matrixordering.h"

#include "point.h"
#include "project.h"
#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/bandwidth.hpp>
#include <boost/graph/cuthill_mckee_ordering.hpp>
#include <boost/graph/properties.hpp>

MatrixOrdering::MatrixOrdering(Project *_project) :
    newOrder(0), project(_project)
{
    //std::cout<<"MatrixOrdering"<<std::endl;
    //just an example...
    /*typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                boost::property<boost::vertex_color_t, boost::default_color_type,
                boost::property<boost::vertex_degree_t,int> > > Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
    typedef boost::graph_traits<Graph>::vertices_size_type size_type;

    typedef std::pair<std::size_t, std::size_t> Pair;
    Pair edges[14] = { Pair(0,3), //a-d
                         Pair(0,5),  //a-f
                         Pair(1,2),  //b-c
                         Pair(1,4),  //b-e
                         Pair(1,6),  //b-g
                         Pair(1,9),  //b-j
                         Pair(2,3),  //c-d
                         Pair(2,4),  //c-e
                         Pair(3,5),  //d-f
                         Pair(3,8),  //d-i
                         Pair(4,6),  //e-g
                         Pair(5,6),  //f-g
                         Pair(5,7),  //f-h
                         Pair(6,7) }; //g-h

    Graph G(10);
    for (int i = 0; i < 14; ++i)
      boost::add_edge(edges[i].first, edges[i].second, G);

    boost::graph_traits<Graph>::vertex_iterator ui, ui_end;

    boost::property_map<Graph,boost::vertex_degree_t>::type deg = get(boost::vertex_degree, G);
    for (boost::tie(ui, ui_end) = vertices(G); ui != ui_end; ++ui)
      deg[*ui] = boost::degree(*ui, G);

    boost::property_map<Graph, boost::vertex_index_t>::type
      index_map = get(boost::vertex_index, G);

    std::cout << "original bandwidth: " << boost::bandwidth(G) << std::endl;

    std::vector<Vertex> inv_perm(num_vertices(G));
    std::vector<size_type> perm(num_vertices(G));

    //reverse cuthill_mckee_ordering
    boost::cuthill_mckee_ordering(G, inv_perm.rbegin(), boost::get(boost::vertex_color, G),
                           boost::make_degree_map(G));

    std::cout << "Reverse Cuthill-McKee ordering:" << std::endl;
    std::cout << "  ";
    for (std::vector<Vertex>::const_iterator i=inv_perm.begin();
       i != inv_perm.end(); ++i)
      std::cout << index_map[*i] << " ";
    std::cout << std::endl;

    for (size_type c = 0; c != inv_perm.size(); ++c)
      perm[index_map[inv_perm[c]]] = c;
    std::cout << "  bandwidth: "
              << boost::bandwidth(G, boost::make_iterator_property_map(&perm[0], index_map, perm[0]))
              << std::endl;*/
}

bool MatrixOrdering::computeRelations()
{
    if (project->points.empty()) return false;

#ifdef REORDER_DEBUG
    std::cout<<"nb points "<<project->points.size()<<std::endl;
    for (auto & point : project->points)
    {
        std::cout<<"Point "<<point.name<<std::endl;
        std::cout<<"     "<<point.getPointNumber()<<std::endl;
    }
#endif
    relations.resize(static_cast<Eigen::Index>(project->points.size()),static_cast<Eigen::Index>(project->points.size()));
    relations.fill(false);

    for (auto & point : project->points)
    {
        relations(point.getPointNumber(),point.getPointNumber())=true;
    }
    for (auto & station : project->stations)
        for (auto & obs : station->observations)
            if (obs.active)
            {
              #ifdef REORDER_DEBUG
                std::cout<<"relations:\n"<<relations<<"\n";
                std::cout<<"from: "<<obs.from<<"   to: "<<obs.to<<std::endl;
                std::cout<<"change: "<<obs.from->getPointNumber()<<" "<<obs.to->getPointNumber()<<std::endl;
              #endif
                relations(obs.from->getPointNumber(),obs.to->getPointNumber())=true;
                relations(obs.to->getPointNumber(),obs.from->getPointNumber())=true;
                //in case of station->from != obs.from
                if ((station->origin()) && (station->origin() != obs.from))
                {
                    relations(station->origin()->getPointNumber(),obs.from->getPointNumber())=true;
                    relations(station->origin()->getPointNumber(),obs.to->getPointNumber())=true;
                    relations(obs.from->getPointNumber(),station->origin()->getPointNumber())=true;
                    relations(obs.to->getPointNumber(),station->origin()->getPointNumber())=true;
                }
            }

#ifdef REORDER_DEBUG
    std::cout<<"Relations before orderning ---------------------"<<std::endl;
    for (unsigned int i=0;i<project->points.size();i++)
    {
        for (unsigned int j=0;j<project->points.size();j++)
            std::cout<<relations(i,j)<<" ";
        std::cout<<std::endl;
    }
    std::cout<<"End relations -------------------"<<std::endl;
#endif
    return true;
}

void MatrixOrdering::orderRelations()
{
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, boost::property<boost::vertex_color_t, boost::default_color_type, boost::property<boost::vertex_degree_t, int>>>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
    using size_type = boost::graph_traits<Graph>::vertices_size_type;

    using Pair = std::pair<std::size_t, std::size_t>;
    std::vector <Pair> edges;
    for (unsigned int i=0;i<project->points.size();i++)
        for (unsigned int j=0;j<project->points.size();j++)
            if (relations(i,j))
                edges.emplace_back(i,j);

    Graph G(project->points.size());
    for (auto & edge : edges)
        boost::add_edge(edge.first, edge.second, G);


    boost::graph_traits<Graph>::vertex_iterator ui, ui_end;

    boost::property_map<Graph,boost::vertex_degree_t>::type deg = get(boost::vertex_degree, G);
    for (boost::tie(ui, ui_end) = vertices(G); ui != ui_end; ++ui)
      deg[*ui] = boost::degree(*ui, G);         // NOLINT: conversion from unsigned long to int

    boost::property_map<Graph, boost::vertex_index_t>::type
      index_map = get(boost::vertex_index, G);

    std::cout << QObject::tr("Original bandwidth: ").toCstr() << boost::bandwidth(G) << std::endl;

    std::vector<Vertex> inv_perm(num_vertices(G));
    std::vector<size_type> perm(num_vertices(G));

    //reverse cuthill_mckee_ordering
    boost::cuthill_mckee_ordering(G, inv_perm.rbegin(), boost::get(boost::vertex_color, G),
                           boost::make_degree_map(G));

    /*std::cout << QObject::tr("Reverse Cuthill-McKee ordering:").toCstr() << std::endl;
    std::cout << "  ";
    for (std::vector<Vertex>::const_iterator i=inv_perm.begin();
       i != inv_perm.end(); ++i)
      std::cout << index_map[*i] << " ";
    std::cout << std::endl;*/

    for (size_type c = 0; c != inv_perm.size(); ++c)
      perm[index_map[inv_perm[c]]] = c;
    std::cout << QObject::tr("Opimized bandwidth: ").toCstr()
              << boost::bandwidth(G, boost::make_iterator_property_map(perm.data(), index_map, perm[0]))
              << std::endl;

    //fill newOrder list
    newOrder.resize(project->points.size());
    unsigned int jj=0;
    for (const auto& i : inv_perm)
    {
        newOrder[jj]=index_map[i];
        jj++;
    }

  #ifdef REORDER_DEBUG
    std::cout<<"New order: ";
    for (unsigned int j=0;j<project->points.size();j++)
        std::cout<<newOrder[j]<<" ";
    std::cout<<std::endl;

    //show new matrix
    std::cout<<"Relations after orderning ---------------------"<<std::endl;
    for (unsigned int i=0;i<project->points.size();i++)
    {
        for (unsigned int j=0;j<project->points.size();j++)
            std::cout<<relations(newOrder[i],newOrder[j])<<" ";
        std::cout<<std::endl;
    }
    std::cout<<"End relations -------------------"<<std::endl;
  #endif
}
