#include "output_handler.h"

OutputHandler::OutputHandler(Parameters* parameters, InputData* input_data, PoolBaseData* male_pool, PoolBaseData* female_pool, bool male_index, bool female_index) {

    // Pointers to data structures from PSASS
    this->input_data = input_data;
    this->male_pool = male_pool;
    this->female_pool = female_pool;
    this->male_index = male_index;
    this->female_index = female_index;

    this->parameters = parameters;

    // Create base output file path
    if (this->parameters->output_prefix != "") this->parameters->output_prefix += "_";

    // Open output file objects
    if (this->parameters->output_fst_pos) this->fst_position_output_file.open(this->parameters->output_prefix);
    if (this->parameters->output_fst_win) this->fst_window_output_file.open(this->parameters->output_prefix);
    if (this->parameters->output_snps_pos) this->snps_position_output_file.open(this->parameters->output_prefix);
    if (this->parameters->output_snps_win) this->snps_window_output_file.open(this->parameters->output_prefix);
    if (this->parameters->output_depth) this->depth_output_file.open(this->parameters->output_prefix);
    if (this->parameters->output_genes) this->genes_output_file.open(this->parameters->output_prefix);
}



// Write SNP and nucleotide information if current base is a sex-specific SNP
void OutputFile::open(const std::string& prefix) {

    this->path = prefix + this->suffix + ".tsv";
    this->file.open(this->path);
    if (not this->file.is_open()) {
        std::cerr << "Error: cannot open output file (" <<this->path << ")." << std::endl;
        exit(1);
    }
    this->file << this->header;
}



// Write Fst information if current base has fst higher than specified threshold
void OutputHandler::output_fst_position(float fst) {

    this->fst_position_output_file.file << std::fixed << std::setprecision(2)
                                        << this->input_data->contig << "\t" << this->input_data->position << "\t" << fst <<  "\n";
}



// Write Fst information for the current windows
void OutputHandler::output_fst_window(float fst) {

    this->fst_window_output_file.file << this->input_data->contig << "\t" << this->input_data->position - this->parameters->window_range << "\t"
                                      << std::fixed << std::setprecision(2)
                                      << fst << "\n";
}



// Write SNP and nucleotide information if current base is a sex-specific SNP
void OutputHandler::output_snp_position(std::string sex) {

    this->snps_position_output_file.file << std::fixed << std::setprecision(2)
                                         << this->input_data->contig << "\t" << this->input_data->position << "\t" << sex << "\t"
                                         << this->male_pool->output_frequencies() << "\t"
                                         << this->female_pool->output_frequencies() << "\n";
}



// Write SNP and nucleotide information for the current window
void OutputHandler::output_snp_window(uint16_t snps_total[2]) {

    this->snps_window_output_file.file << this->input_data->contig << "\t" << this->input_data->position - this->parameters->window_range << "\t"
                                       << snps_total[this->male_index] << "\t"
                                       << snps_total[this->female_index] << "\n";
}


// Write depth information at the end of the analysis
void OutputHandler::output_depth(std::map<std::string, std::map<uint, float[2]>>& depth, float* average_depth) {

    uint window_size = 0;

    for (auto const& contig : depth) {

        for (auto const& position: contig.second) {

            window_size = std::min(position.first + this->parameters->window_range, this->parameters->window_size);

            this->depth_output_file.file << contig.first << "\t" << position.first << "\t"
                                         << float(position.second[this->male_index] / window_size) << "\t"
                                         << float(position.second[this->female_index] / window_size) << "\t"
                                         << std::fixed << std::setprecision(2)
                                         << float((position.second[this->male_index] / window_size)/ average_depth[0]) << "\t"
                                         << float((position.second[this->female_index] / window_size)/ average_depth[1]) << "\n";

        }
    }
}


// Write genes information at the end of the analysis
void OutputHandler::output_genes(std::unordered_map<std::string, Gene>& genes, float* average_depth) {

    float depth_correction_males = (average_depth[0] + average_depth[1]) / 2 / average_depth[0];
    float depth_correction_females = (average_depth[0] + average_depth[1]) / 2 / average_depth[1];

    uint gene_length = 0, male_depth = 0, female_depth = 0;

    for (auto gene: genes) {

        gene_length = uint(std::stoi(gene.second.end) -  std::stoi(gene.second.start));
        male_depth = (gene.second.depth[4]) / gene_length;
        female_depth = (gene.second.depth[5]) / gene_length;
        gene.second.noncoding_length = gene_length - gene.second.coding_length;

        if (gene.second.coding_length == 0) {

            gene.second.depth[0] = 0;
            gene.second.depth[2] = 0;

        } else {

            gene.second.depth[0] /= gene.second.coding_length;
            gene.second.depth[2] /= gene.second.coding_length;

        }

        if (gene.second.noncoding_length == 0) {

            gene.second.depth[1] = 0;
            gene.second.depth[3] = 0;

        } else {

            gene.second.depth[1] /= gene.second.noncoding_length;
            gene.second.depth[3] /= gene.second.noncoding_length;

        }

        this->genes_output_file.file << gene.second.contig << "\t" << gene.second.start << "\t" << gene.second.end << "\t"
                                     << gene.second.name << "\t" << gene.second.product << "\t"
                                     << male_depth << "\t" << int(male_depth * depth_correction_males) << "\t"
                                     << gene.second.depth[0] << "\t" << int(gene.second.depth[0] * depth_correction_males) << "\t"
                                     << gene.second.depth[1] << "\t" << int(gene.second.depth[1] * depth_correction_males) << "\t"
                                     << female_depth << "\t" << int(female_depth * depth_correction_females) << "\t"
                                     << gene.second.depth[2] << "\t" << int(gene.second.depth[2] * depth_correction_females) << "\t"
                                     << gene.second.depth[3] << "\t" << int(gene.second.depth[3] * depth_correction_females) << "\t"
                                     << gene.second.snps[4] << "\t" << gene.second.snps[0] << "\t" << gene.second.snps[1] << "\t"
                                     << gene.second.snps[5] << "\t" << gene.second.snps[2] << "\t" << gene.second.snps[3] << "\n";
    }

}