/*
 * io.cpp
 *
 *  Created on: 24 de nov. de 2024
 *      Author: Alexandre
 */
#include "simulation.h"
#include <cstring>

namespace automaton
{
	// Serialize SN
	void serializeSN(const SN& sn, std::ofstream& out)
	{
		if (!out)
			throw std::ios_base::failure("Failed to open output stream.");
		out.write(reinterpret_cast<const char*>(&sn.a), sizeof(sn.a));
		out.write(reinterpret_cast<const char*>(&sn.s), sizeof(sn.s));
	}

	// Deserialize SN
	void deserializeSN(SN& sn, std::ifstream& in)
	{
		if (!in)
			throw std::ios_base::failure("Failed to open input stream.");
		in.read(reinterpret_cast<char*>(&sn.a), sizeof(sn.a));
		in.read(reinterpret_cast<char*>(&sn.s), sizeof(sn.s));
	}

	// Serialize Cell
	void Cell::serialize(std::ofstream& out) const
	{
		if (!out) throw std::ios_base::failure("Failed to open output stream.");

		out.write(reinterpret_cast<const char*>(&pole), sizeof(pole));
		out.write(reinterpret_cast<const char*>(&charge), sizeof(charge));
		out.write(reinterpret_cast<const char*>(s), sizeof(s));
		out.write(reinterpret_cast<const char*>(&aff), sizeof(aff));
		out.write(reinterpret_cast<const char*>(pos), sizeof(pos));

		out.write(reinterpret_cast<const char*>(&wv), sizeof(wv));
		out.write(reinterpret_cast<const char*>(&d), sizeof(d));
		out.write(reinterpret_cast<const char*>(&sin), sizeof(sin));
		serializeSN(A, out);
		out.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
		out.write(reinterpret_cast<const char*>(&angle), sizeof(angle));

		out.write(reinterpret_cast<const char*>(c), sizeof(c));
		out.write(reinterpret_cast<const char*>(&k), sizeof(k));
		out.write(reinterpret_cast<const char*>(&t), sizeof(t));
		out.write(reinterpret_cast<const char*>(&ctrl), sizeof(ctrl));
		out.write(reinterpret_cast<const char*>(&reloc), sizeof(reloc));

		out.write(reinterpret_cast<const char*>(m), sizeof(m));
		out.write(reinterpret_cast<const char*>(&e), sizeof(e));
		serializeSN(A_bar, out);
		serializeSN(ph, out);

		out.write(reinterpret_cast<const char*>(&collapse), sizeof(collapse));
		out.write(reinterpret_cast<const char*>(&net_c0), sizeof(net_c0));
		out.write(reinterpret_cast<const char*>(&net_c1), sizeof(net_c1));
		out.write(reinterpret_cast<const char*>(&net_c2), sizeof(net_c2));
		out.write(reinterpret_cast<const char*>(&net_q), sizeof(net_q));
		out.write(reinterpret_cast<const char*>(&net_w0), sizeof(net_w0));
		out.write(reinterpret_cast<const char*>(&net_w1), sizeof(net_w1));
		out.write(reinterpret_cast<const char*>(&fxf), sizeof(fxf));
		out.write(reinterpret_cast<const char*>(&bxb), sizeof(bxb));
		out.write(reinterpret_cast<const char*>(&fxb), sizeof(fxb));
		out.write(reinterpret_cast<const char*>(&wxw), sizeof(wxw));
		out.write(reinterpret_cast<const char*>(&boson), sizeof(boson));
	}

	// Deserialize Cell
	void Cell::deserialize(std::ifstream& in)
	{
		if (!in) throw std::ios_base::failure("Failed to open input stream.");

		in.read(reinterpret_cast<char*>(&pole), sizeof(pole));
		in.read(reinterpret_cast<char*>(&charge), sizeof(charge));
		in.read(reinterpret_cast<char*>(s), sizeof(s));
		in.read(reinterpret_cast<char*>(&aff), sizeof(aff));
		in.read(reinterpret_cast<char*>(pos), sizeof(pos));

		in.read(reinterpret_cast<char*>(&wv), sizeof(wv));
		in.read(reinterpret_cast<char*>(&d), sizeof(d));
		in.read(reinterpret_cast<char*>(&sin), sizeof(sin));
		deserializeSN(A, in);
		in.read(reinterpret_cast<char*>(&freq), sizeof(freq));
		in.read(reinterpret_cast<char*>(&angle), sizeof(angle));

		in.read(reinterpret_cast<char*>(c), sizeof(c));
		in.read(reinterpret_cast<char*>(&k), sizeof(k));
		in.read(reinterpret_cast<char*>(&t), sizeof(t));
		in.read(reinterpret_cast<char*>(&ctrl), sizeof(ctrl));
		in.read(reinterpret_cast<char*>(&reloc), sizeof(reloc));

		in.read(reinterpret_cast<char*>(m), sizeof(m));
		in.read(reinterpret_cast<char*>(&e), sizeof(e));
		deserializeSN(A_bar, in);
		deserializeSN(ph, in);

		in.read(reinterpret_cast<char*>(&collapse), sizeof(collapse));
		in.read(reinterpret_cast<char*>(&net_c0), sizeof(net_c0));
		in.read(reinterpret_cast<char*>(&net_c1), sizeof(net_c1));
		in.read(reinterpret_cast<char*>(&net_c2), sizeof(net_c2));
		in.read(reinterpret_cast<char*>(&net_q), sizeof(net_q));
		in.read(reinterpret_cast<char*>(&net_w0), sizeof(net_w0));
		in.read(reinterpret_cast<char*>(&net_w1), sizeof(net_w1));
		in.read(reinterpret_cast<char*>(&fxf), sizeof(fxf));
		in.read(reinterpret_cast<char*>(&bxb), sizeof(bxb));
		in.read(reinterpret_cast<char*>(&fxb), sizeof(fxb));
		in.read(reinterpret_cast<char*>(&wxw), sizeof(wxw));
		in.read(reinterpret_cast<char*>(&boson), sizeof(boson));
	}

	/*
	 * Save the current state of the CA in disk.
	 */
	void saveState0()
	{
	    std::ofstream out("CA-state0.dat", std::ios::binary);
	    if (!out)
	    {
	        std::cerr << "Error opening file for writing!" << std::endl;
	        return;
	    }
	    size_t count = lattice_current.size();
	    out.write(reinterpret_cast<const char*>(&count), sizeof(count));  // Save number of objects
	    for (const auto& cell : lattice_current)
	    {
	        cell.serialize(out);
	    }
	    out.close();
	}

	bool compareCells(const Cell& cell1, const Cell& cell2)
	{
	    // Compare scalar members
	    if (cell1.pole != cell2.pole || cell1.charge != cell2.charge || cell1.aff != cell2.aff)
	    {
	        return false;
	    }
	    // Compare arrays (if s, pos, m, etc., are arrays or containers)
	    if (std::memcmp(cell1.s, cell2.s, sizeof(cell1.s)) != 0 ||
	        std::memcmp(cell1.pos, cell2.pos, sizeof(cell1.pos)) != 0 ||
	        std::memcmp(cell1.m, cell2.m, sizeof(cell1.m)) != 0)
	    {
	        return false;
	    }
	    // Add other comparisons as needed
	    return true; // All comparisons passed
	}

	void checkPoincare()
	{
		std::ifstream in("CA-state0.dat", std::ios::binary);  // Open file in binary mode
		if (!in)
		{
			std::cerr << "Error opening file for reading!" << std::endl;
			return;
		}
		size_t count;
		in.read(reinterpret_cast<char*>(&count), sizeof(count));  // Read number of objects
	    Cell saved_cell;
	    bool success = true;
	    for (const auto& cell : lattice_current)
		{
	        saved_cell.deserialize(in);
	        if (!compareCells(cell, saved_cell))
	        	success = false;
		}
		in.close();
		//
        if (success)
        	puts("Poincaré Success.");
        else
        	puts("Poincaré Failed.");
	}

}


