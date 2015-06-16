// -*- mode: c++; indent-tabs-mode: nil; -*-
//
// Manta
// Copyright (c) 2013-2015 Illumina, Inc.
//
// This software is provided under the terms and conditions of the
// Illumina Open Source Software License 1.
//
// You should have received a copy of the Illumina Open Source
// Software License 1 along with this program. If not, see
// <https://github.com/sequencing/licenses/>
//

///
/// \author Xiaoyu Chen
/// \author Felix Schlesinger
///

#pragma once

#include "blt_util/known_pos_range2.hh"
#include "blt_util/qscore_snp.hh"
#include "htsapi/bam_record.hh"

#include <cstdint>

#include <string>
#include <iosfwd>


struct SRAlignmentInfo
{
    unsigned alignPos = 0;
    unsigned leftSize = 0;
    unsigned homSize = 0;
    unsigned rightSize = 0;
    unsigned leftMismatches = 0;
    unsigned homMismatches = 0;
    unsigned rightMismatches = 0;
    unsigned alignScore = 0;
    float alignLnLhood = 0;

    bool isEvidence = false;
    bool isTier2Evidence = false;
    float evidence = 0;
};

std::ostream&
operator<<(std::ostream& os, const SRAlignmentInfo& info);


///
/// \param[in] flankScoreSize the number of bases to score past the end of microhomology range
///
/// \param[in] targetBpOffsetRange this is the range of the breakend (accounting for microhomology) in targetSeq coordinates
///
/// TODO: need to add a query subset/length limit, so that as the query size goes up (ie. 2 x 400) we still consistently
///       detect split read support without having to add more and more reference to the targetSeq
///
void
splitReadAligner(
    const unsigned flankScoreSize,
    const std::string& querySeq,
    const qscore_snp& qualConvert,
    const uint8_t* queryQual,
    const std::string& targetSeq,
    const known_pos_range2& targetBpOffsetRange,
    SRAlignmentInfo& alignment);

/// Populate an SRAlignmentInfo object based on the existing alignment of the bamRead to the genomic region around this break-end.
/// Scores the alignment based on (mis-)match counts and likelihood as the splitReadAligner.
///
/// \param[in] bpPos this is the range of the breakend (accounting for microhomology) in genome coordinates
///
void
getRefAlignment(
    const bam_record& bamRead,
    const reference_contig_segment& bp1ref,
    const known_pos_range2& bpPos,
    const qscore_snp& qualConvert,
    SRAlignmentInfo& alignment);
