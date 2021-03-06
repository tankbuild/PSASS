#include "psass.h"

// Psass class constructor
Psass::Psass(int argc, char *argv[]) {

    this->t_begin = std::chrono::steady_clock::now();

    ArgParser cmd_options(argc, argv);
    cmd_options.set_parameters(this->parameters);

    this->logs = Logs(this->parameters.output_prefix);
    this->logs.write("PSASS started.");
    cmd_options.output_parameters(logs.file);

    if (this->parameters.male_pool == 1) {

        this->male_pool = &this->pair_data.pool1;
        this->female_pool = &this->pair_data.pool2;

    } else {

        this->male_pool = &this->pair_data.pool2;
        this->female_pool = &this->pair_data.pool1;

    }

    this->male_index = (this->parameters.male_pool == 2);  // 0 if male pool is first, 1 otherwise
    this->female_index = (this->parameters.male_pool == 1);  // 0 if male pool is second, 1 otherwise

    this->output_handler = OutputHandler(&this->parameters, &this->input_data, this->male_pool, this->female_pool, this->male_index, this->female_index,
                                         &this->depth_data, &this->gff_data.genes, &this->logs);

    std::cout << "Preprocessing data ..." << std::endl;
    if (this->parameters.output_genes) {

        this->gff_data.read_gff_file(this->parameters.gff_file, this->logs);
        this->parameters.output_genes = true;

    }

    this->count_lines();
}



void Psass::count_lines() {

    do {

        this->parameters.input_file.read(this->input_data.buff, this->input_data.buff_size);
        this->input_data.k = this->parameters.input_file.gcount();

        for (uint i=0; i<this->input_data.k; ++i) {

            switch (this->input_data.buff[i]) {

                case '\n':
                    ++this->input_data.total_lines;
                    break;

                default:
                    break;

            }
        }

    } while (parameters.input_file);

    this->input_data.lines_percent = this->input_data.total_lines / 100;

    this->parameters.input_file.clear();
    this->parameters.input_file.seekg(0);

}



// Update window_base_data.nucleotides
void Psass::update_fst_parts() {

    // Fst computation for the window is implemented using the formula described in Karlsson et al 2007
    // https://www.nature.com/articles/ng.2007.10   https://doi.org/10.1038/ng.2007.10

    uint8_t n_alleles[2];
    uint32_t allele_m = 0, allele_f = 0;
    float a1 = 0.0, a2 = 0.0, n1 = 0.0, n2 = 0.0;
    float h1 = 0.0, h2 = 0.0, N = 0.0, D = 0.0;

    if (this->male_pool->depth > this->parameters.min_depth and this->female_pool->depth > this->parameters.min_depth) {

        n_alleles[0] = 0;
        n_alleles[1] = 0;
        allele_m = 0;
        allele_f = 0;
        a1 = 0;
        a2 = 0;
        n1 = 0;
        n2 = 0;
        h1 = 0;
        h2 = 0;
        N = 0;
        D = 0;

        for (uint i=0; i<6; ++i) {

            if (this->male_pool->nucleotides[i] > 0) {

                ++n_alleles[this->male_index];
                allele_m = this->male_pool->nucleotides[i];
                allele_f = this->female_pool->nucleotides[i];

            }

            if (this->female_pool->nucleotides[i] > 0) {

                ++n_alleles[this->female_index];
            }
        }

        if (n_alleles[0] == 2 and n_alleles[1] == 2) {

            a1 = float(allele_m);
            a2 = float(allele_f);
            n1 = float(this->male_pool->depth);
            n2 = float(this->female_pool->depth);

            h1 = a1 * (n1 - a1) / (n1 * (n1 - 1));
            h2 = a2 * (n2 - a2) / (n2 * (n2 - 1));
            N = (a1 / n1 - a2 / n2) * (a1 / n1 - a2 / n2) - h1 / n1 - h2 / n2;
            D = N + h1 + h2;

        }
    }

    this->window_base_data.fst_parts[0] = N;
    this->window_base_data.fst_parts[1] = D;

}



// Check whether current position is a sex-specific SNPs for each sex and update window_base_data.snps
void Psass::update_snps() {

    this->window_base_data.snps[this->male_index] = false;
    this->window_base_data.snps[this->female_index] = false;

    if (this->male_pool->depth > this->parameters.min_depth and this->female_pool->depth > this->parameters.min_depth) {

        for (auto i=0; i<6; ++i) {

            if (this->male_pool->frequencies[i] > this->parameters.min_het and
                this->male_pool->frequencies[i] < this->parameters.max_het and
                this->female_pool->frequencies[i] > this->parameters.min_hom) {

                if (this->parameters.group_snps) {

                    if (not this->consecutive_snps[this->male_index]) this->window_base_data.snps[this->male_index] = true;
                    else this->window_base_data.snps[this->male_index] = false;
                    this->consecutive_snps[this->male_index] = true;

                } else {

                    this->window_base_data.snps[this->male_index] = true;

                }

            }

            if (this->female_pool->frequencies[i] > this->parameters.min_het and
                this->female_pool->frequencies[i] < this->parameters.max_het and
                this->male_pool->frequencies[i] > this->parameters.min_hom) {

                if (this->parameters.group_snps) {

                    if (not this->consecutive_snps[this->female_index]) this->window_base_data.snps[this->female_index] = true;
                    else this->window_base_data.snps[this->female_index] = false;
                    this->consecutive_snps[this->female_index] = true;

                } else {

                    this->window_base_data.snps[this->female_index] = true;

                }

            }
        }

        if (not this->window_base_data.snps[this->male_index] and not this->window_base_data.snps[this->female_index]) {

            this->consecutive_snps[this->male_index] = false;
            this->consecutive_snps[this->female_index] = false;

        }
    }
}



// Update window_base_data.coverage from each pool's data depth
void Psass::update_depth() {

    // Update data to push in window
    if (this->parameters.output_depth) {

        this->window_base_data.depth[this->male_index] = this->male_pool->depth;
        this->window_base_data.depth[this->female_index] = this->female_pool->depth;

    }

    // Update total depth count to compute relative coverage later
    if (this->parameters.output_depth or this->parameters.output_genes) {

        this->total_depth[this->male_index] += this->male_pool->depth;
        this->total_depth[this->female_index] += this->female_pool->depth;

    }
}



// Update sliding window data
void Psass::update_window(bool end) {

    // Add the current base data to the window (reset window if size bigger than window_size, which should not happen)
    (this->window.data.size() <= this->parameters.window_size and not end) ? this->window.data.push_back(this->window_base_data) : this->window.data.resize(0);

    // If the window is smaller than window_size, only add to total (beginning of the contig)
    if (this->window.data.size() <= this->parameters.window_size) {

        if (this->parameters.output_snps_win) {

            this->window.snps_in_window[this->male_index] += this->window_base_data.snps[this->male_index];
            this->window.snps_in_window[this->female_index] += this->window_base_data.snps[this->female_index];

        }

        if (this->parameters.output_depth) {

            this->window.depth_in_window[this->male_index] += this->window_base_data.depth[this->male_index];
            this->window.depth_in_window[this->female_index] += this->window_base_data.depth[this->female_index];

        }

        if (this->parameters.output_fst_win) {

            this->window.fst_parts[0] += this->window_base_data.fst_parts[0];
            this->window.fst_parts[1] += this->window_base_data.fst_parts[1];

        }


    } else if (this->window.data.size() == this->parameters.window_size + 1) {  // Normal case (within contig) : substract front, add new value, remove front.

        if (this->parameters.output_fst_win) {

            this->window.fst_parts[0] = this->window.fst_parts[0]
                                        - this->window.data[0].fst_parts[0]
                                        + this->window_base_data.fst_parts[0];

            this->window.fst_parts[1] = this->window.fst_parts[1]
                                        - this->window.data[0].fst_parts[1]
                                        + this->window_base_data.fst_parts[1];

        }

        if (this->parameters.output_snps_win) {

            this->window.snps_in_window[this->male_index] = this->window.snps_in_window[this->male_index]
                                                            - this->window.data[0].snps[this->male_index]
                                                            + this->window_base_data.snps[this->male_index];

            this->window.snps_in_window[this->female_index] = this->window.snps_in_window[this->female_index]
                                                              - this->window.data[0].snps[this->female_index]
                                                              + this->window_base_data.snps[this->female_index];

        }

        if (this->parameters.output_depth) {

            this->window.depth_in_window[this->male_index] = this->window.depth_in_window[this->male_index]
                                                             - this->window.data[0].depth[this->male_index]
                                                             + this->male_pool->depth;

            this->window.depth_in_window[this->female_index] = this->window.depth_in_window[this->female_index]
                                                               - this->window.data[0].depth[this->female_index]
                                                               + this->female_pool->depth;

        }

        this->window.data.pop_front();

    }
}



// Update genes data
void Psass::update_genes() {

    std::string gene = "", contig = "";
    bool coding = false;

    if (this->gff_data.contig.find(this->input_data.position) != this->gff_data.contig.end()) {

        gene = this->gff_data.contig[this->input_data.position].first;
        coding = this->gff_data.contig[this->input_data.position].second;

        // Coding or non-coding depth and snps
        this->gff_data.genes[gene].depth[2 * this->male_index + coding] += this->male_pool->depth;
        this->gff_data.genes[gene].depth[2 * this->female_index + coding] += this->female_pool->depth;
        this->gff_data.genes[gene].snps[2 * this->male_index + coding] += this->window_base_data.snps[this->male_index];
        this->gff_data.genes[gene].snps[2 * this->female_index + coding] += this->window_base_data.snps[this->female_index];

        // Gene-level depth and snps
        this->gff_data.genes[gene].depth[4 + this->male_index] += this->male_pool->depth;
        this->gff_data.genes[gene].depth[4 + this->female_index] += this->female_pool->depth;
        this->gff_data.genes[gene].snps[4 + this->male_index] += this->window_base_data.snps[this->male_index];
        this->gff_data.genes[gene].snps[4 + this->female_index] += this->window_base_data.snps[this->female_index];

    }
}



// Handles output of sliding window
void Psass::output_window_step() {

    if (this->parameters.output_fst_win) this->output_handler.output_fst_window(this->window.fst_parts);

    if (this->parameters.output_snps_win) this->output_handler.output_snp_window(this->window.snps_in_window);

    if (this->parameters.output_depth) {

        this->depth_data[this->input_data.contig][this->input_data.position - this->parameters.window_range][this->male_index] = this->window.depth_in_window[this->male_index];
        this->depth_data[this->input_data.contig][this->input_data.position - this->parameters.window_range][this->female_index] = this->window.depth_in_window[this->female_index];
        this->depth_data[this->input_data.contig][this->input_data.position - this->parameters.window_range][2] = float(this->window.data.size());

    }
}



// End of contig needs special processing. Progressively remove the beginning of the window until last position.
void Psass::process_contig_end() {

    uint last_spot = uint(this->input_data.last_position / this->parameters.output_resolution) * this->parameters.output_resolution + this->parameters.window_range;
    uint first_spot = last_spot - this->parameters.window_range;
    auto tmp = this->window.data;

    uint32_t tmp_position = this->input_data.position;
    std::string tmp_contig = this->input_data.contig;

    this->input_data.contig = this->input_data.current_contig;

    for (auto i = first_spot; i <= last_spot; i += this->parameters.output_resolution) {

        this->window.fst_parts[0] = 0;
        this->window.fst_parts[1] = 0;
        this->window.snps_in_window[0] = 0;
        this->window.snps_in_window[1] = 0;
        this->window.depth_in_window[0] = 0;
        this->window.depth_in_window[1] = 0;

        this->input_data.position = i;

        for (uint j = 0; j < this->parameters.output_resolution; ++j) tmp.pop_front();

        for (auto base: tmp) {

            this->window.snps_in_window[0] += base.snps[0];
            this->window.snps_in_window[1] += base.snps[1];
            this->window.depth_in_window[0] += base.depth[0];
            this->window.depth_in_window[1] += base.depth[1];
            this->window.fst_parts[0] += base.fst_parts[0];
            this->window.fst_parts[1] += base.fst_parts[1];
        }

        this->window.data = tmp;

        this->output_window_step();

    }

    this->input_data.position = tmp_position;
    this->input_data.contig = tmp_contig;
}



// Function called on a line from the input file (i.e. when meeting a '\n')
void Psass::process_line() {

    // Fill last pool2 base
    this->pair_data.pool2.nucleotides[5] = fast_stoi(this->input_data.temp.c_str());

    // Reset values
    this->pair_data.fst = this->male_index;
    this->window_base_data.snps[this->male_index] = false;
    this->window_base_data.snps[this->female_index] = false;

    // Update / reset values if change of contig
    if (this->input_data.contig != this->input_data.current_contig) {

        if (this->input_data.current_contig != "") {

           if(this->window.data.size() > this->parameters.window_size) this->process_contig_end();

           this->logs.write("Processing of contig <" + this->input_data.current_contig + "> ended without errors.");
           this->logs.write("Processing of contig <" + this->input_data.contig + "> started.");

            if (parameters.output_snps_win) {

                this->window.snps_in_window[this->male_index] = 0;
                this->window.snps_in_window[this->female_index] = 0;

            }

            if (parameters.output_depth) {

                this->window.depth_in_window[this->male_index] = 0;
                this->window.depth_in_window[this->female_index] = 0;

            }

            if (parameters.output_fst_win) {

                this->window.fst_parts[0] = 0;
                this->window.fst_parts[1] = 0;

            }

            this->window.data.resize(0);

        } else {

            this->logs.write("Processing of contig <" + this->input_data.contig + "> started.");

        }

        if (parameters.output_genes) this->gff_data.new_contig(this->input_data, this->logs);

        if (parameters.group_snps) {

            this->consecutive_snps[0] = false;
            this->consecutive_snps[1] = false;

        }
    }

    // Reset line parsing values
    this->input_data.current_contig = this->input_data.contig;
    this->input_data.last_position = this->input_data.position;
    this->input_data.field = 0;
    this->input_data.temp = "";

    // Update data (depth per pool, fst, pi ...)
    this->pair_data.update(this->parameters.output_fst_pos);

    // Update nucleotides data for window
    if (this->parameters.output_fst_win) this->update_fst_parts();

    // Update depth data for window
    if (this->parameters.output_depth or this->parameters.output_genes) this->update_depth();

    // Update SNPs data for window
    if (this->parameters.output_snps_win or this->parameters.output_snps_pos or this->parameters.output_genes) this->update_snps();

    // Update window data
    if (this->parameters.output_fst_win or this->parameters.output_snps_win or this->parameters.output_depth) this->update_window();

    // Update genes data
    if (this->parameters.output_genes) this->update_genes();

    ++this->total_bases;

    if (this->total_bases % this->input_data.lines_percent == 0) {

        this->input_data.completion_line = "\rProcessing file : ";
        for (ulong i = 0; i <= this->total_bases / this->input_data.lines_percent; ++i) this->input_data.completion_line += "\u2588";
        for (ulong i = this->total_bases / this->input_data.lines_percent + 1; i < 101; ++i) this->input_data.completion_line += "\u00B7";
        this->input_data.completion_line += " [" + std::to_string(this->total_bases / this->input_data.lines_percent) + "%]";
        std::cout << this->input_data.completion_line << std::flush;

    }

    // Output Fst positions
    if (parameters.output_fst_pos) {

        if (this->pair_data.fst > this->parameters.min_fst) this->output_handler.output_fst_position(this->pair_data.fst);
    }

    // Output SNPs positions
    if (parameters.output_snps_pos) {

        if (this->window_base_data.snps[this->male_index]) this->output_handler.output_snp_position("M");
        if (this->window_base_data.snps[this->female_index]) this->output_handler.output_snp_position("F");

    }

    // Output window information and update coverage
    if ((this->input_data.position == 1 or (this->input_data.position - this->parameters.window_range) % this->parameters.output_resolution == 0)
        and this->input_data.position >= this->parameters.window_range) {

        this->output_window_step();

    }
}



// Function called on a field from the input file (i.e. when meeting a '\t')
void Psass::process_field() {

    switch (this->input_data.field) {

    case 0:
        this->input_data.contig = this->input_data.temp;
        break;

    case 1:
        this->input_data.position = fast_stoi(this->input_data.temp.c_str());
        break;

    case 2:
        break;

    case 3:
        this->pair_data.pool1.nucleotides[5] = fast_stoi(this->input_data.temp.c_str());
        break;

    default:
        break;

    }

    this->input_data.temp = "";
    this->input_data.subfield = 0;
    ++this->input_data.field;
}



// Function called on a subfield from the input file (i.e. when meeting a ':')
void Psass::process_subfield() {

    switch (this->input_data.field) {

    case 3:
        this->pair_data.pool1.nucleotides[this->input_data.subfield] = fast_stoi(this->input_data.temp.c_str());
        break;

    case 4:
        this->pair_data.pool2.nucleotides[this->input_data.subfield] = fast_stoi(this->input_data.temp.c_str());
        break;

    default:
        break;
    }
    this->input_data.temp = "";
    ++this->input_data.subfield;
}



// Read the input file and process each line
void Psass::run() {

    this->logs.write("Processing of <" + this->parameters.input_file_path + "> started.");
    std::cout << "PSASS started." << std::endl;

    do {

        this->parameters.input_file.read(this->input_data.buff, this->input_data.buff_size);
        this->input_data.k = this->parameters.input_file.gcount();

        for (uint i=0; i<this->input_data.k; ++i) {

            switch (this->input_data.buff[i]) {

                case '\r':
                    break;

                case '\n':
                    this->process_line();
                    break;

                case '\t':
                    this->process_field();
                    break;

                case ':':
                    this->process_subfield();
                    break;

                default:
                    this->input_data.temp += this->input_data.buff[i];
                    break;

            }
        }

    } while (parameters.input_file);

    this->logs.write("Processing of <" + this->parameters.input_file_path + "> ended without errors.");
    this->logs.write("Processed <" + std::to_string(this->total_bases) + "> lines.");

    this->average_depth[this->male_index] = float(this->total_depth[this->male_index]) / float(this->total_bases);
    this->average_depth[this->female_index] = float(this->total_depth[this->female_index]) / float(this->total_bases);

    if (this->parameters.output_depth) this->output_handler.output_depth(this->average_depth);
    if (this->parameters.output_genes) this->output_handler.output_genes(this->average_depth);

    std::chrono::steady_clock::time_point t_end = std::chrono::steady_clock::now();
    long seconds = std::chrono::duration_cast<std::chrono::seconds>(t_end - t_begin).count();
    long minutes = seconds / 60;
    long hours = minutes / 60;
    this->logs.write("Total runtime : " + std::to_string(hours) + "h " + std::to_string(minutes%60) + "m " + std::to_string(seconds%60) + "s.");
    this->logs.write("PSASS ended without errors.");

}
