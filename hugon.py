#!/usr/bin/env python
#coding: utf-8

import re
import sys

_sx=re.compile(r'^(-?[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?)(.*)$')


class HugonHandler():
	def setData(self,data,is_key):
		if is_key:
			print "Key",data
		else:
			print "Data",data
	
	def startObject(self):
		print "{"
		
	def endObject(self):
		print "}"
	
	def startArray(self):
		print "["
		
	def endArray(self):
		print "]"

	def startValue(self):
		print "("
	
	def endValue(self):
		print ")"




class HugonParser():
	def __init__(self,handler):
		self._string=''
		self._token=None
		self._stack='?'
		self._last_token=None
		self._handler=handler
	
	def close(self):
		if self._string != '' or self._stack != '?':
			raise AttributeError('Ucięty stream')
		self._token=None
		self._last_token=None
			
	def feed(self,data):
		self._string += data
		while self._get_token():
			self._push_token()
			
	def _push_token(self):
		# key end
		if self._stack[-1] == '{' and (self._token == ',' or self._token == '}'):
			if self._last_token != '{':
				self._send_token(')')
		# null element in array
		if self._stack[-1] == '[' and (self._token==',' or self._token==']'):
			if self._token == ',' and (self._last_token == ',' or self._last_token=='['):
				self._send_token('null')
			elif self._token == ']' and self._last_token == ',':
				self._send_token('null')
		# start key
		is_key=False
		if self._stack[-1] == '{' and not self._token in "{}[],:":
			is_key = self._last_token == '{' or self._last_token == ','
				
		self._last_token=self._token
		self._send_token(self._token,is_key)
	
	def _send_token(self,token,is_key=False):
		if token == ',':
			return
		if token == '{':
			self._handler.startObject()
		elif token == '}':
			self._handler.endObject()
		elif token == '[':
			self._handler.startArray()
		elif token == ']':
			self._handler.endArray()
		elif token == ':':
			self._handler.startValue()
		elif token == ')':
			self._handler.endValue()
		else:
			if token.startswith('d'):
				token=token[1:]
			self._handler.setData(token,is_key)
	
	def _get_token(self):
		global _sx
		self._string=self._string.lstrip()
		if self._string == '':
			return False
		znak=self._string[0]
		if znak in '{}[],:':
			if znak in '{[':
				self._stack += znak
			elif znak in '}]':
				if self._stack[-1] != ('{' if znak == '}' else '['):
					print znak,self._stack
					raise AttributeError()
				self._stack=self._stack[:-1]
			self._token=znak
			self._string=self._string[1:]
			return True
		if znak == '\'' or znak == '"':
			le=len(self._string)
			dc=False
			for i in range(1,le):
				if dc:
					dc=False
					continue
				if self._string[i]=='\\':
					dc=True
					continue
				if self._string[i]==znak:
					self._token=self._string[:i+1]
					self._string=self._string[i+1:]
					return True
			return False
		for i in ('true','false','null'):
			if self._string.startswith(i):
				self._token=i
				self._string=self._string[len(i):]
				return True
		g=_sx.match(self._string)
		if not g:
			return False
		if len(g.group(4)) == 0:
			return False
		self._token='d'+g.group(1)
		self._string=g.group(4)

	        return True

			
		
	



handler=HugonHandler()
parser=HugonParser(handler)
f=open(sys.argv[1])
for i in range(1,500):
	parser.feed(f.read(10000))
parser.close()
