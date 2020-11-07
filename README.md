# hugon

Hugon is JSON stream parsing library dedicated for parsing huge (not fitting in memory) JSON data. It was written primarily upon my request by ethanak in Python, then rewritten to C++ by me. It behaves like SAX parser. Neither serializing nor deserializing features, just JSON stream parser based upon callbacks. Very useful when parsing huge JSON data, or somehow corrupted JSON data stream.
