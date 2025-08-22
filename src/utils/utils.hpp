#include <algorithm>
#include <vector>
#include <memory>

#ifndef __list_defined
#define __list_defined

class LIST
{

public:
	LIST(int32_t res = 0)
	{
		if (res)
			m_eids.reserve(res);
	}

	inline uint32_t operator[](int32_t index) const
	{
		return m_eids[index];
	}

	inline std::vector<uint32_t>& eids()
	{
		return m_eids;
	}

	inline int32_t count() const
	{
		return static_cast<int32_t>(m_eids.size());
	}

	inline void add(uint32_t eid)
	{
		if (!m_eids.empty() && std::find(m_eids.begin(), m_eids.end(), eid) != m_eids.end())
			return;

		m_eids.push_back(eid);
	}

	inline void remove(uint32_t eid)
	{
		if (m_eids.empty())
			return;
		auto it = std::find(m_eids.begin(), m_eids.end(), eid);
		if (it == m_eids.end())
			return;

		m_eids.erase(it);
	}

	inline int32_t find(uint32_t eid) const
	{
		if (m_eids.empty())
			return -1;

		auto it = std::find(m_eids.begin(), m_eids.end(), eid);
		if (it == m_eids.end())
			return -1;

		return static_cast<int32_t>(it - m_eids.begin());

	}

	inline static bool is_subset(const LIST& subset, const LIST& superset)
	{
		for (const auto& item : subset.m_eids)
		{
			if (superset.find(item) == -1)
				return false;
		}
		return true;
	}

	inline void copy_in(const LIST& other)
	{
		for (auto& eid : other.m_eids)
		{
			if (std::find(m_eids.begin(), m_eids.end(), eid) == m_eids.end())
				m_eids.push_back(eid);
		}
	}

	inline void remove_all(const LIST& other)
	{
		for (auto& eid : other.m_eids)
		{
			auto it = std::find(m_eids.begin(), m_eids.end(), eid);
			if (it != m_eids.end())
				m_eids.erase(it);
		}
	}

private:
	std::vector<uint32_t> m_eids{};
};

class DEPENDENCY
{

public:
	DEPENDENCY(int32_t t = 0, int32_t s = 0)
	{
		type = t;
		subtype = s;
	}

	int32_t type = 0;
	int32_t subtype = 0;
	LIST dependencies{};
};

class MATRIX_STORED_LL
{
public:
	MATRIX_STORED_LL(uint32_t z, int32_t path)
	{
		zone = z;
		cam_path = path;
	}

	uint32_t zone = 0;
	int32_t cam_path = 0;
	LIST full_load{};
};

// used to store load or draw list in its non-delta form
class LOAD
{
public:
	int8_t type;	 // 'A' or 'B'
	int32_t index;   // point index
	LIST list{};
};


using LOAD_LIST = std::vector<LOAD>;
using MATRIX_STORED_LLS = std::vector<MATRIX_STORED_LL>;
using DEPENDENCIES = std::vector<DEPENDENCY>;

#endif
