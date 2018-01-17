/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/utils/reports/nrnreport.h"
#include "coreneuron/utils/reports/nrnsection_mapping.h"
#include "coreneuron/nrniv/nrn_assert.h"
#include "coreneuron/nrnoc/mech_mapping.hpp"
#include "coreneuron/utils/tokenizer.hpp"
#ifdef ENABLE_REPORTING
#include "reportinglib/Records.h"
#endif

extern int nrn_get_mechtype(const char*);
static std::vector<ReportGenerator*> reports;
/*
 * ---                           ---
 *     Public API implementation
 * ---                           ---
 */

void register_report(int type,
                     double start,
                     double stop,
                     double dt,
                     double delay,
                     double dt_report,
                     std::string report_path,
                     std::string report_filter) {
#ifdef ENABLE_REPORTING
    reports.push_back(new ReportGenerator(type, start, stop, dt, delay, dt_report, report_path, report_filter));
    reports[reports.size()-1]->register_report();
#else
    if (nrnmpi_myid == 0)
       printf("\n WARNING! : Can't enable reports, recompile with ReportingLib! \n");
#endif

}

void finalize_report () {
  for (std::vector<ReportGenerator*>::iterator it = reports.begin(); it != reports.end(); it++) {
    ReportGenerator* report = *it;
  // C++11 for (auto report: reports) {
    if (report) delete report;
  }
  reports.clear();
}

void ReportEvent::selectGIDtoReport (NrnThread* nt , int gid) {
   gids_to_report[nt].push_back(gid);
}

/** constructor */
ReportEvent::ReportEvent(double t) {
    dt = t;
    step = 0;
}

/** on deliver, call ReportingLib and setup next event */
void ReportEvent::deliver(double t, NetCvode* nc, NrnThread* nt) {
// avoid pgc++-Fatal-/opt/pgi/linux86-64/16.5/bin/pggpp2ex TERMINATED by signal 11
#ifdef ENABLE_REPORTING

/** @todo: reportinglib is not thread safe, fix this */
// clang-format off
    #pragma omp critical
    // clang-format on
    {
// each thread needs to know its own step
#ifdef ENABLE_REPORTING
        records_nrec(step, gids_to_report[nt].size(), &gids_to_report[nt][0]);
#endif
        send(t + dt, nc, nt);
        step++;
    }
#else
    (void)t;
    (void)nc;
    (void)nt;
#endif  // ENABLE_REPORTING
}

/*
 * filter string from command line in the following format:
 *  VAR(, VAR)*
 */
 void ReportGenerator::parseFilterString (std::string filter_list) {
   std::vector<std::string> tokens = tokenize(filter_list.c_str(), ',');
   for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++) {
   std::string& element = *it;
  //C++11 for (auto&& element : tokenize(filter_list.c_str(), ',')){
    std::vector<std::string> tok (tokenize(element.c_str(), '.'));
    int type                     = nrn_get_mechtype (tok[0].c_str());
    std::string var_name         = tok[1];
    if (filters.find(type) == filters.end()) {
        filters[type] =  std::vector<std::string> ();
        filters[type].push_back(var_name);
    }
    else
        filters[type].push_back(var_name);
  }
}

/** based on command line arguments, setup reportinglib interface */
ReportGenerator::ReportGenerator(int rtype,
                                 double start_,
                                 double stop_,
                                 double dt_,
                                 double delay,
                                 double dt_report_,
                                 std::string path, 
                                 std::string filter_str) {
    start         = start_;
    stop          = stop_;
    dt            = dt_;
    dt_report     = dt_report_;
    mindelay      = delay;
    parseFilterString (filter_str);
    switch (rtype) {
        case 1:
            type = SomaReport;
            report_filepath = path + std::string("/soma");
            break;

        case 2:
            type = CompartmentReport;
            report_filepath = path + std::string("/voltage");
            break;

        case 3:
            type = SynapseReport;
            report_filepath = path + std::string("/synapse");
            break;
        default:
            if (nrnmpi_myid == 0) {
                std::cout << " WARNING: Invalid report type, enabling Soma reports!\n";
            }
            type = SomaReport;
            report_filepath = path + std::string("/soma");
    }
}

bool ReportGenerator::something_need_to_be_reported (NrnThread& nt, std::vector<int>& nodes_gid) {
    bool result = false;
    NrnThreadMappingInfo* mapinfo = (NrnThreadMappingInfo*)nt.mapping;
    if (! nt.ncell) return false;
    switch (type) {
      case CompartmentReport:
      case SomaReport:
        result = true;
        break;
      default:
        for (int i = 0; i < nt.ncell; ++i) {
          int gid = nt.presyns[i].gid_;
          for (ReportFilter::iterator filter = filters.begin(); filter != filters.end(); filter++) {
          //C++11  for (const auto&  filter : filters) {
               int synapse_type = filter->first;
               Memb_list *ml = nt._ml_list[synapse_type];
               if (ml && ml->nodecount) {
                 int local_gid = nodes_gid[ml->nodeindices[i]];
                 if (local_gid != gid) continue;
                 for (int i = 0; i <  ml->nodecount; i++)  {
                  if (*getVarLocationFromVarName (synapse_type, "selected_for_report", ml, i)) {
                    result = true;
                    break;
                  }
                 }
                 if (result == true) break;
               }
           }
        }
    }
    return result;
}

bool ReportGenerator::synapseReportThisGID (NrnThread& nt, std::vector<int>& nodes_gid, int gid) {
  bool result = false;
  for (ReportFilter::iterator filter = filters.begin(); filter != filters.end(); filter++) {
  // C++11 for (const auto&  filter : filters) {
      int synapse_type = filter->first;
      Memb_list *ml = nt._ml_list[synapse_type];
      if (ml && ml->nodecount) {
        for (int i = 0; i <  ml->nodecount; i++)  {
           int local_gid = nodes_gid[ml->nodeindices[i]];
           if (local_gid != gid) continue;
           if (*getVarLocationFromVarName (synapse_type, "selected_for_report", ml, i)) {
             result = true;
             break;
           }
        }
        if (result == true) break;
      }
  }
  return result;
}
// register GIDs for every compartment, it consist in a backward sweep then forward sweep algorithm
std::vector<int> ReportGenerator::registerGIDs (NrnThread& nt) {
     std::vector<int> nodes_gid (nt.end, -1);
     // backward sweep: from presyn compartment propagate back GID to parent
     for(int i=0; i < nt.n_presyn; i++) {
         int gid = nt.presyns[i].gid_;
         int thvar_index = nt.presyns[i].thvar_index_;
         // only for non artificial cells
         if (thvar_index >= 0) {
             // setting all roots gids of the presyns nodes,
             // index 0 have parent set to 0, so we must stop at j > 0
             // also 0 is the parent of all, so it is an error to attribute a GID to it.
               nodes_gid[thvar_index]  = gid;
               for (int j = thvar_index; j > 0; j=nt._v_parent_index[j]) {
                       nodes_gid[nt._v_parent_index[j]] = gid;
               }
         }
     }
     // forward sweep: setting all compartements nodes to the GID of its root, 
     //  already setup up on above loop, and it is working only because compartments are stored just in order to parents
     for(int i=nt.ncell + 1; i < nt.end; i++) {
             nodes_gid[i] = nodes_gid[nt._v_parent_index[i]];
     }
  return nodes_gid;
}

#ifdef ENABLE_REPORTING
void ReportGenerator::register_report() {
    /* simulation dt */
    records_set_atomic_step(dt);
    std::map<int,bool> registered;
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        std::vector<int> nodes_gid = registerGIDs(nt);
        NrnThreadMappingInfo* mapinfo = (NrnThreadMappingInfo*)nt.mapping;

        /** dont create events on NrnThread  that dont need to report anything
         *  this is also requirement from reportinglib
         * */
        if ( something_need_to_be_reported(nt, nodes_gid)) {
            events.push_back(new ReportEvent(dt));
            events[ith]->send(t, net_cvode_instance, &nt);

            /** @todo: hard coded parameters for ReportingLib from Jim*/
            int sizemapping = 1;
            int extramapping = 5;
            int mapping[] = {0};            // first column i.e. section numbers
            int extra[] = {1, 0, 0, 0, 1};  // first row, from 2nd value (skip gid)
            const char* unit = "mV";
            const char* kind = "compartment";
            const char* reportname = report_filepath.c_str();

            int segment_count = 0;
            int section_count = 0;
            int extra_node = 0;

            /** iterate over all neurons */
            for (int i = 0; i < nt.ncell; ++i) {
                /** for this gid, get mapping information */
                int gid = nt.presyns[i].gid_;
                int nsections = 0;
                int nsegments = 0;
                CellMapping* m = mapinfo->get_cell_mapping(gid);
                if ((type == CompartmentReport) || (type == SomaReport)) {
                  if (m == NULL) {
                      std::cout << "Error : Compartment mapping information is missing! \n";
                      continue;
                  }
                  nsections = m->num_segments();
                  nsegments = m->num_sections();

                  section_count += nsections;
                  segment_count += nsegments;
                  extra_node++;
                }
                /** for full compartment reports, set extra mapping */
                if (type == CompartmentReport) {
                    extra[0] = nsegments;
                    extra[1] = m->get_seclist_segment_count("soma");
                    extra[2] = m->get_seclist_segment_count("axon");
                    extra[3] = m->get_seclist_segment_count("dend");
                    extra[4] = m->get_seclist_segment_count("apic");
                }
                if (type == CompartmentReport || type == SomaReport || synapseReportThisGID(nt, nodes_gid, gid)) {
                  events[ith]->selectGIDtoReport(&nt, gid);

                  /** add report variable : @todo api changes in reportinglib*/
                  records_add_report((char*)reportname, gid, gid, gid, start, stop, dt_report,
                                   sizemapping, (char*)kind, extramapping, (char*)unit);

                  /** add extra mapping : @todo api changes in reportinglib*/
                  records_extra_mapping((char*)reportname, gid, 5, extra);
                }
                if (type == SomaReport) {
                    /** get  section list mapping for soma */
                    SecMapping* s = m->get_seclist_mapping("soma");

                    /** 1st key is section-id and 1st value is segment of soma */
                    mapping[0] = s->secmap.begin()->first;
                    int idx = s->secmap.begin()->second.front();

                    /** corresponding voltage in coreneuron voltage array */
                    double* v = nt._actual_v + idx;

                    /** add segment for reporting */
                    records_add_var_with_mapping((char*)reportname, gid, v, sizemapping, mapping);
                    // sum of sections and segments plus one initial extra node
                    // should be equal to number of nodes in coreneuron.
                } else if (type == CompartmentReport) {
                    for (size_t j = 0; j < m->size(); j++) {
                        SecMapping* s = m->secmapvec[j];

                        for (secseg_it_type iterator = s->secmap.begin();
                             iterator != s->secmap.end(); iterator++) {
                            mapping[0] = iterator->first;
                            segvec_type& vec = iterator->second;

                            for (size_t k = 0; k < vec.size(); k++) {
                                int idx = vec[k];

                                /** corresponding voltage in coreneuron voltage array */
                                double* v = nt._actual_v + idx;

                                /** add segment for reporting */
                                records_add_var_with_mapping((char*)reportname, gid, v, sizemapping,
                                                             mapping);
                            }
                        }
                    }
                }
                else { //synapse report
                    for (ReportFilter::iterator filter = filters.begin();  filter != filters.end(); filter++) {
                    // C++11 for (const auto&  filter : filters) {
                      int synapse_type = filter->first;
                      Memb_list *ml = nt._ml_list[synapse_type];
                      //TODO if all nt dont have an ml: emit a message to user to tell that it try to report something that is
                      //      not existing in loaded model
                      if (! ml ) {
                       continue;
                      }
                          for (size_t j = 0; j < filter->second.size(); j++) {
                            std::string& var_name = filter->second[j];
                      // C++11 for (const auto& var_name : filter.second) {
                          for (size_t i = 0; i < ml->nodecount; i++) {
                               int local_gid = nodes_gid[ml->nodeindices[i]];
                               if (local_gid != gid) continue;
                               if(*getVarLocationFromVarName (synapse_type, "selected_for_report", ml, i)) {
                                registered[gid] = true;
                                double* var_value   =  getVarLocationFromVarName (synapse_type, var_name.c_str(), ml, i); // pointer to synapse variable
                                double* synapse_id  =  getVarLocationFromVarName (synapse_type, "synapseID", ml, i);
                                mapping[0] = *synapse_id;
                                records_add_var_with_mapping((char*)reportname, gid, var_value, sizemapping, mapping);
                              }
                          }
                      }
                    }
                }
            }
            events[ith]->sortSelectedGIDs();
        }
    }
    
    /** in the current implementation, we call flush during every spike exchange
     *  interval. Hence there should be sufficient buffer to hold all reports
     *  for the duration of mindelay interval. In the below call we specify the
     *  number of timesteps that we have to buffer.
     *  TODO: revisit this because spike exchange can happen few steps before/after
     *  mindelay interval and hence adding two extra timesteps to buffer.
     */
    int timesteps_to_buffer = mindelay / dt_report + 2;
    records_set_steps_to_buffer(timesteps_to_buffer);

    /** reportinglib setup */
    records_setup_communicator();
    records_finish_and_share();

    if (nrnmpi_myid == 0) {
        if (type == SomaReport)
            std::cout << " Soma report registration finished!\n";
        else if (type == CompartmentReport)
            std::cout << " Full compartment report registration finished!\n";
        else
           std::cout << " Synapse report registration finished ! \n";
    }
}

extern "C" void nrn_flush_reports(double t) {
    records_flush(t);
}

#endif  // ENABLE_REPORTING
